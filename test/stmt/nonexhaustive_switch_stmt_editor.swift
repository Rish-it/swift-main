// RUN: %target-typecheck-verify-swift -diagnostics-editor-mode -enable-library-evolution -enable-nonfrozen-enum-exhaustivity-diagnostics

public enum NonExhaustive {
  case a, b
}

// Inlineable code is considered "outside" the module and must include a default
// case.
@inlinable
public func testNonExhaustive(_ value: NonExhaustive) {
  switch value { // expected-error {{switch must be exhaustive}}
  // expected-note@-1 {{add missing cases}}
  case .a: break
  }

  // expected-warning@+2 {{switch covers known cases, but 'NonExhaustive' may have additional unknown values}}
  // expected-note@+1 {{handle unknown values using "@unknown default"}} {{+3:3-3=@unknown default:\n<#fatalError()#>\n}}
  switch value {
  case .a: break
  case .b: break
  }

  // expected-error@+2 {{switch must be exhaustive}}
  // expected-note@+1 {{add missing cases}} {{+1:3-3=case .a:\n<#code#>\ncase .b:\n<#code#>\n@unknown default:\n<#code#>\n}}
  switch value {
  }

  switch value {
  case .a: break
  case .b: break
  default: break // no-warning
  }

  switch value {
  case .a: break
  case .b: break
  @unknown case _: break // no-warning
  }
}
