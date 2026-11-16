// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: depth = 1
// CHECK: loop-carried = 0
// CHECK: vector-plan:
// CHECK-NEXT: decision = candidate
// CHECK: vectorization = legal
// CHECK-LABEL: mopt.loop #1
// CHECK: depth = 0
// CHECK: loop-carried = 1
// CHECK: kind = WAW
// CHECK-NEXT: certainty = proven
// CHECK-NEXT: distance = 1
// CHECK: vector-plan:
// CHECK-NEXT: decision = rejected
// CHECK: reasons = [target-width-and-element-type, loop-carried-dependence]
// CHECK: vectorization = illegal

module {
  func.func @nested_dependence(%A: memref<16xf32>, %value: f32) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c16 = arith.constant 16 : index

    scf.for %i = %c0 to %c16 step %c1 {
      scf.for %j = %c0 to %c16 step %c1 {
        memref.store %value, %A[%j] : memref<16xf32>
      }
    }

    return
  }
}
