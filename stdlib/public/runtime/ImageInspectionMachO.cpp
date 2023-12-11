//===--- ImageInspectionMachO.cpp - Mach-O image inspection ---------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// \file
///
/// This file includes routines that interact with dyld on Mach-O-based
/// platforms to extract runtime metadata embedded in images generated by the
/// Swift compiler.
///
//===----------------------------------------------------------------------===//

#if defined(__APPLE__) && defined(__MACH__) &&                                 \
    !defined(SWIFT_RUNTIME_STATIC_IMAGE_INSPECTION)

#include "ImageInspection.h"
#include "ImageInspectionCommon.h"
#include "swift/Runtime/Config.h"
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#if SWIFT_OBJC_INTEROP
#include <objc/runtime.h>
#endif
#include <assert.h>
#include <dlfcn.h>

#if __has_include(<objc/objc-internal.h>) && __has_include(<mach-o/dyld_priv.h>)
#include <mach-o/dyld_priv.h>
#include <objc/objc-internal.h>
#else

// Bring our own definition of enum _dyld_section_location_kind and some of its
// values. They'll be unused when we can't include dyld_priv.h, but the code
// needs them in order to compile.
enum _dyld_section_location_kind {
  _dyld_section_location_text_swift5_protos,
  _dyld_section_location_text_swift5_proto,
  _dyld_section_location_text_swift5_types,
  _dyld_section_location_text_swift5_replace,
  _dyld_section_location_text_swift5_replace2,
  _dyld_section_location_text_swift5_ac_funcs,
};

#endif

// Bring our own definition of _dyld_section_location constants in case we're
// using an older SDK that doesn't have them.
#define _dyld_section_location_text_swift5_protos 0
#define _dyld_section_location_text_swift5_proto 1
#define _dyld_section_location_text_swift5_types 2
#define _dyld_section_location_text_swift5_replace 3
#define _dyld_section_location_text_swift5_replace2 4
#define _dyld_section_location_text_swift5_ac_funcs 5

#if OBJC_ADDLOADIMAGEFUNC2_DEFINED
// Redefine _dyld_lookup_section_info as weak so we can build against it but
// still run when it's not present at runtime. Note that we don't have to check
// for its presence at runtime, as it's guaranteed to be available if we get
// the callbacks from objc_addLoadImageFunc2.
LLVM_ATTRIBUTE_WEAK
struct _dyld_section_info_result
_dyld_lookup_section_info(const struct mach_header *mh,
                          _dyld_section_location_info_t locationHandle,
                          enum _dyld_section_location_kind kind);
#else
// If we don't have objc_addLoadImageFunc2, fall back to objc_addLoadImageFunc.
// Use a #define so we don't have to conditionalize the calling code below.
#define objc_addLoadImageFunc2 objc_addLoadImageFunc
#endif

using namespace swift;

namespace {

constexpr const char ProtocolsSection[] = MachOProtocolsSection;
constexpr const char ProtocolConformancesSection[] =
    MachOProtocolConformancesSection;
constexpr const char TypeMetadataRecordSection[] =
    MachOTypeMetadataRecordSection;
constexpr const char DynamicReplacementSection[] =
    MachODynamicReplacementSection;
constexpr const char DynamicReplacementSomeSection[] =
    MachODynamicReplacementSomeSection;
constexpr const char AccessibleFunctionsSection[] =
    MachOAccessibleFunctionsSection;
constexpr const char TextSegment[] = MachOTextSegment;

#if __POINTER_WIDTH__ == 64
using mach_header_platform = mach_header_64;
#else
using mach_header_platform = mach_header;
#endif

// Callback for objc_addLoadImageFunc that just takes a mach_header.
template <const char *SEGMENT_NAME, const char *SECTION_NAME, int SECTION_KIND,
          void CONSUME_BLOCK(const void *baseAddress, const void *start,
                             uintptr_t size)>
void addImageCallback(const mach_header *mh) {
#if __POINTER_WIDTH__ == 64
  assert(mh->magic == MH_MAGIC_64 && "loaded non-64-bit image?!");
#endif

  unsigned long size;
  const uint8_t *section =
  getsectiondata(reinterpret_cast<const mach_header_platform *>(mh),
                 SEGMENT_NAME, SECTION_NAME,
                 &size);

  if (!section)
    return;

  CONSUME_BLOCK(mh, section, size);
}

// Callback for objc_addLoadImageFunc2 that takes a mach_header and dyld info.
#if OBJC_ADDLOADIMAGEFUNC2_DEFINED
template <const char *SEGMENT_NAME, const char *SECTION_NAME, int SECTION_KIND,
          void CONSUME_BLOCK(const void *baseAddress, const void *start,
                             uintptr_t size)>
void addImageCallback(const mach_header *mh,
                      struct _dyld_section_location_info_s *dyldInfo) {
#if __POINTER_WIDTH__ == 64
  assert(mh->magic == MH_MAGIC_64 && "loaded non-64-bit image?!");
#endif

  auto result = _dyld_lookup_section_info(
      mh, dyldInfo,
      static_cast<enum _dyld_section_location_kind>(SECTION_KIND));
  if (result.buffer)
    CONSUME_BLOCK(mh, result.buffer, result.bufferSize);
}
#endif

// Callback for _dyld_register_func_for_add_image that takes a mach_header and a
// slide.
template <const char *SEGMENT_NAME, const char *SECTION_NAME, int SECTION_KIND,
          void CONSUME_BLOCK(const void *baseAddress, const void *start,
                             uintptr_t size)>
void addImageCallback(const mach_header *mh, intptr_t vmaddr_slide) {
  addImageCallback<SEGMENT_NAME, SECTION_NAME, SECTION_KIND, CONSUME_BLOCK>(mh);
}

// Callback for objc_addLoadImageFunc that just takes a mach_header.

template <const char *SEGMENT_NAME, const char *SECTION_NAME,
          const char *SEGMENT_NAME2, const char *SECTION_NAME2,
          int SECTION_KIND, int SECTION_KIND2,
          void CONSUME_BLOCK(const void *baseAddress, const void *start,
                             uintptr_t size, const void *start2,
                             uintptr_t size2)>
void addImageCallback2Sections(const mach_header *mh) {
#if __POINTER_WIDTH__ == 64
  assert(mh->magic == MH_MAGIC_64 && "loaded non-64-bit image?!");
#endif

  // Look for a section.
  unsigned long size;
  const uint8_t *section =
  getsectiondata(reinterpret_cast<const mach_header_platform *>(mh),
                 SEGMENT_NAME, SECTION_NAME,
                 &size);

  if (!section)
    return;

  // Look for another section.
  unsigned long size2;
  const uint8_t *section2 =
  getsectiondata(reinterpret_cast<const mach_header_platform *>(mh),
                 SEGMENT_NAME2, SECTION_NAME2,
                 &size2);
  if (!section2)
    size2 = 0;

  CONSUME_BLOCK(mh, section, size, section2, size2);
}

// Callback for objc_addLoadImageFunc2 that takes a mach_header and dyld info.
#if OBJC_ADDLOADIMAGEFUNC2_DEFINED
template <const char *SEGMENT_NAME, const char *SECTION_NAME,
          const char *SEGMENT_NAME2, const char *SECTION_NAME2,
          int SECTION_KIND, int SECTION_KIND2,
          void CONSUME_BLOCK(const void *baseAddress, const void *start,
                             uintptr_t size, const void *start2,
                             uintptr_t size2)>
void addImageCallback2Sections(const mach_header *mh,
                               struct _dyld_section_location_info_s *dyldInfo) {
#if __POINTER_WIDTH__ == 64
  assert(mh->magic == MH_MAGIC_64 && "loaded non-64-bit image?!");
#endif

  // Look for a section.
  auto result = _dyld_lookup_section_info(
      mh, dyldInfo,
      static_cast<enum _dyld_section_location_kind>(SECTION_KIND));
  if (!result.buffer)
    return;

  auto result2 = _dyld_lookup_section_info(
      mh, dyldInfo,
      static_cast<enum _dyld_section_location_kind>(SECTION_KIND2));
  // No NULL check here, we allow this one not to be present. dyld gives us
  // a NULL pointer and 0 size when the section isn't in the dylib so we don't
  // need to zero anything out.

  CONSUME_BLOCK(mh, result.buffer, result.bufferSize, result2.buffer,
                result2.bufferSize);
}
#endif

// Callback for _dyld_register_func_for_add_image that takes a mach_header and a
// slide.
template <const char *SEGMENT_NAME, const char *SECTION_NAME,
          const char *SEGMENT_NAME2, const char *SECTION_NAME2,
          int SECTION_KIND, int SECTION_KIND2,
          void CONSUME_BLOCK(const void *baseAddress, const void *start,
                             uintptr_t size, const void *start2,
                             uintptr_t size2)>
void addImageCallback2Sections(const mach_header *mh, intptr_t vmaddr_slide) {
  addImageCallback2Sections<SEGMENT_NAME, SECTION_NAME, SEGMENT_NAME2,
                            SECTION_NAME2, SECTION_KIND, SECTION_KIND2,
                            CONSUME_BLOCK>(mh);
}

} // end anonymous namespace

#if OBJC_ADDLOADIMAGEFUNC_DEFINED && SWIFT_OBJC_INTEROP
#define REGISTER_FUNC(...)                                                     \
  if (SWIFT_RUNTIME_WEAK_CHECK(objc_addLoadImageFunc2)) {                      \
    SWIFT_RUNTIME_WEAK_USE(objc_addLoadImageFunc2(__VA_ARGS__));               \
  } else if (__builtin_available(macOS 10.15, iOS 13, tvOS 13,                 \
                                 watchOS 6, *)) {                              \
    objc_addLoadImageFunc(__VA_ARGS__);                                        \
  } else {                                                                     \
    _dyld_register_func_for_add_image(__VA_ARGS__);                            \
  }
#else
#define REGISTER_FUNC(...) _dyld_register_func_for_add_image(__VA_ARGS__)
#endif

// WARNING: the callbacks are called from unsafe contexts (with the dyld and
// ObjC runtime locks held) and must be very careful in what they do. Locking
// must be arranged to avoid deadlocks (other code must never call out to dyld
// or ObjC holding a lock that gets taken in one of these callbacks) and the
// new/delete operators must not be called, in case a program supplies an
// overload which does not cooperate with these requirements.

void swift::initializeProtocolLookup() {
  REGISTER_FUNC(addImageCallback<TextSegment, ProtocolsSection,
                                 _dyld_section_location_text_swift5_protos,
                                 addImageProtocolsBlockCallbackUnsafe>);
}

void swift::initializeProtocolConformanceLookup() {
  REGISTER_FUNC(
      addImageCallback<TextSegment, ProtocolConformancesSection,
                       _dyld_section_location_text_swift5_proto,
                       addImageProtocolConformanceBlockCallbackUnsafe>);
}
void swift::initializeTypeMetadataRecordLookup() {
  REGISTER_FUNC(
      addImageCallback<TextSegment, TypeMetadataRecordSection,
                       _dyld_section_location_text_swift5_types,
                       addImageTypeMetadataRecordBlockCallbackUnsafe>);
}

void swift::initializeDynamicReplacementLookup() {
  REGISTER_FUNC(
      addImageCallback2Sections<TextSegment, DynamicReplacementSection,
                                TextSegment, DynamicReplacementSomeSection,
                                _dyld_section_location_text_swift5_replace,
                                _dyld_section_location_text_swift5_replace2,
                                addImageDynamicReplacementBlockCallback>);
}

void swift::initializeAccessibleFunctionsLookup() {
  REGISTER_FUNC(
      addImageCallback<TextSegment, AccessibleFunctionsSection,
                       _dyld_section_location_text_swift5_ac_funcs,
                       addImageAccessibleFunctionsBlockCallbackUnsafe>);
}

#endif // defined(__APPLE__) && defined(__MACH__) &&
       // !defined(SWIFT_RUNTIME_STATIC_IMAGE_INSPECTION)
