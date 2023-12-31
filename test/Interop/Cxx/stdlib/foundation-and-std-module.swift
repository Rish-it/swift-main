// RUN: %empty-directory(%t)
// RUN: %target-swift-frontend %s -c -enable-experimental-cxx-interop -Xcc -std=c++14 -Xcc -fmodules-cache-path=%t
// RUN: %target-swift-frontend %s -c -enable-experimental-cxx-interop -Xcc -std=c++17 -Xcc -fmodules-cache-path=%t
// RUN: %target-swift-frontend %s -c -enable-experimental-cxx-interop -Xcc -std=c++20 -Xcc -fmodules-cache-path=%t -Xcc -D_LIBCPP_DISABLE_AVAILABILITY

// RUN: find %t | %FileCheck %s

// RUN: %empty-directory(%t)

// RUN: %target-swift-frontend %s -c -enable-experimental-cxx-interop -Xcc -std=c++17 -Xcc -fmodules-cache-path=%t -DADD_CXXSTDLIB
// RUN: %target-swift-frontend %s -c -enable-experimental-cxx-interop -Xcc -std=c++20 -Xcc -fmodules-cache-path=%t -DADD_CXXSTDLIB -Xcc -D_LIBCPP_DISABLE_AVAILABILITY
// FIXME: remove _LIBCPP_DISABLE_AVAILABILITY above (https://github.com/apple/swift/issues/67841)

// REQUIRES: OS=macosx || OS=linux-gnu

#if canImport(Foundation)
import Foundation
#endif

#if ADD_CXXSTDLIB
import CxxStdlib
#endif

func test() {
#if ADD_CXXSTDLIB
  let _ = std.string()
#endif
}

// CHECK: std-{{.*}}.pcm
