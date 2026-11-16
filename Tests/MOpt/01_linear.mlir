// RUN: %mopt %s | FileCheck %s
// RUN: %mopt --dump-ir %s | FileCheck %s --check-prefix=DUMP
// CHECK-LABEL: mopt.loop #0
// CHECK: trip-count = 4096
// CHECK: accesses = 3
// CHECK: target:
// CHECK-NEXT: fixed-vector-bits = 128
// CHECK-NEXT: widest-memory-element-bits = 32
// CHECK-NEXT: vector-plan:
// CHECK-NEXT: decision = unknown
// CHECK-NEXT: natural-factor = 4
// CHECK-NEXT: vector-factor = 4
// CHECK: reasons = [target-width-and-element-type, unknown-memory-dependence]
// CHECK: vectorization = unknown
// DUMP: // ----- mopt IR after pipeline -----
// DUMP-NEXT: module {
// DUMP: func.func @linear

module {
  func.func @linear(
      %A: memref<4096xf32>,
      %B: memref<4096xf32>,
      %C: memref<4096xf32>) {

    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c4096 = arith.constant 4096 : index

    scf.for %i = %c0 to %c4096 step %c1 {
      %a = memref.load %A[%i] : memref<4096xf32>
      %b = memref.load %B[%i] : memref<4096xf32>

      %x = arith.mulf %a, %b : f32
      %y = arith.addf %x, %a : f32

      memref.store %y, %C[%i] : memref<4096xf32>
    }

    return
  }
}
