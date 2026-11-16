// RUN: %mopt --dump-target-info %s | FileCheck %s
// RUN: %mopt --dump-target-info --mcpu=generic %s | FileCheck %s --check-prefix=GENERIC
// RUN: not %mopt --dump-target-info --target-triple=not-a-real-target %s 2>&1 | FileCheck %s --check-prefix=ERROR
// CHECK-LABEL: mopt.target
// CHECK: configuration:
// CHECK: triple = {{.+}}
// CHECK: cpu = {{.+}}
// CHECK: data-layout:
// CHECK: pointer-bits = {{[1-9][0-9]*}}
// CHECK: registers:
// CHECK: fixed-vector-bits = {{[1-9][0-9]*}}
// CHECK: load-store-vector-bits = {{[1-9][0-9]*}}
// CHECK: cache:
// GENERIC-LABEL: mopt.target
// GENERIC: cpu = generic
// ERROR: mopt: failed to query LLVM target information:

module {
}
