// RUN: %target-typecheck-verify-swift -enable-experimental-feature Extern -emit-fixits-path %t.remap -fixit-all -diagnostics-editor-mode
// RUN: c-arcmt-test %t.remap | arcmt-test -verify-transformed-files %s.result

@_extern(c) // expected-warning {{C name '+' may be invalid; explicitly specify the name in @_extern(c) to suppress this warning}}
func +(a: Int, b: Bool) -> Bool
