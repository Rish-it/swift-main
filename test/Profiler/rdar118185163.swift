// RUN: %{python} %S/../Inputs/timeout.py 10 %target-swift-frontend -profile-generate -profile-coverage-mapping -emit-sil -module-name rdar118185163 %s | %FileCheck %s

// rdar://118185163 - Make sure we can generate coverage for this in reasonable
// time.
// CHECK-LABEL: sil_coverage_map {{.*}} "$s13rdar1181851633fooyyF"
func foo() {
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
  do { if .random() {} }
}