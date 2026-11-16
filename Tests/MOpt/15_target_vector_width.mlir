// RUN: %mopt --mcpu=generic %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: target:
// CHECK-NEXT: fixed-vector-bits = 128
// CHECK-NEXT: widest-memory-element-bits = 64
// CHECK-NEXT: vector-plan:
// CHECK-NEXT: decision = unknown
// CHECK-NEXT: natural-factor = 2
// CHECK-NEXT: vector-factor = 2
// CHECK: reasons = [target-width-and-element-type, unknown-memory-dependence]
// CHECK: vectorization = unknown

module {
  func.func @target_vector_width(
      %A: memref<128xf64>,
      %B: memref<128xf64>) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c128 = arith.constant 128 : index

    scf.for %i = %c0 to %c128 step %c1 {
      %value = memref.load %A[%i] : memref<128xf64>
      memref.store %value, %B[%i] : memref<128xf64>
    }

    return
  }
}
