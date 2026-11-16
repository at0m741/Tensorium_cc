// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: step = 2
// CHECK: loop-carried = 0
// CHECK: vector-plan:
// CHECK-NEXT: decision = candidate
// CHECK: reasons = [target-width-and-element-type, legality-allows-vectorization]
// CHECK: vectorization = legal
// CHECK-LABEL: mopt.loop #1
// CHECK: step = 2
// CHECK: loop-carried = 1
// CHECK: kind = RAW
// CHECK-NEXT: certainty = proven
// CHECK-NEXT: distance = 1
// CHECK: vector-plan:
// CHECK-NEXT: decision = rejected
// CHECK: reasons = [target-width-and-element-type, loop-carried-dependence]
// CHECK: vectorization = illegal

module {
  func.func @step2_independent(%A: memref<16xf32>) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c2 = arith.constant 2 : index
    %c16 = arith.constant 16 : index

    scf.for %i = %c0 to %c16 step %c2 {
      %x = memref.load %A[%i] : memref<16xf32>
      %ip1 = arith.addi %i, %c1 : index
      memref.store %x, %A[%ip1] : memref<16xf32>
    }

    return
  }

  func.func @step2_dependent(%A: memref<16xf32>) {
    %c0 = arith.constant 0 : index
    %c2 = arith.constant 2 : index
    %c14 = arith.constant 14 : index

    scf.for %i = %c0 to %c14 step %c2 {
      %x = memref.load %A[%i] : memref<16xf32>
      %ip2 = arith.addi %i, %c2 : index
      memref.store %x, %A[%ip2] : memref<16xf32>
    }

    return
  }
}
