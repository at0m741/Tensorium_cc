// RUN: %mopt --mcpu=generic %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: trip-count = 1
// CHECK: loop-carried = 0
// CHECK: vector-plan:
// CHECK-NEXT: decision = rejected
// CHECK-NEXT: natural-factor = 4
// CHECK-NEXT: vector-factor = 1
// CHECK: reasons = [target-width-and-element-type, trip-count-limits-factor, legality-allows-vectorization, vector-factor-is-one]
// CHECK: vectorization = legal

module {
  func.func @single_iteration(%A: memref<1xf32>) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index

    scf.for %i = %c0 to %c1 step %c1 {
      %value = memref.load %A[%i] : memref<1xf32>
      memref.store %value, %A[%i] : memref<1xf32>
    }

    return
  }
}
