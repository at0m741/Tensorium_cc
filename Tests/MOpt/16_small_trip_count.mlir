// RUN: %mopt --mcpu=generic %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: trip-count = 3
// CHECK: vector-plan:
// CHECK-NEXT: decision = unknown
// CHECK-NEXT: natural-factor = 4
// CHECK-NEXT: vector-factor = 2
// CHECK: reasons = [target-width-and-element-type, trip-count-limits-factor, unknown-memory-dependence]

module {
  func.func @small_trip_count(
      %A: memref<3xf32>,
      %B: memref<3xf32>) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c3 = arith.constant 3 : index

    scf.for %i = %c0 to %c3 step %c1 {
      %value = memref.load %A[%i] : memref<3xf32>
      memref.store %value, %B[%i] : memref<3xf32>
    }

    return
  }
}
