// RUN: %empty-directory(%t)
// RUN: split-file --leading-lines %s %t

//--- a.swift

struct Foo {}
// RUN: %sourcekitd-test -req=related-idents -pos=%(line + 1):6 %t/a.swift -- %t/a.swift | %FileCheck %s
func +(x: Foo, y: Foo) {}
Foo() + Foo()

//--- dummy.swift

// CHECK: START RANGES
// CHECK: 8:6 - 1 - source.syntacticrename.definition
// CHECK: 9:7 - 1 - source.syntacticrename.call
// CHECK: END RANGES
// CHECK: NAME: +(_:_:)