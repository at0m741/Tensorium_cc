// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: loads = 3
// CHECK: stores = 1
// CHECK: accesses = 4

module {
  func.func @stencil_1d(
      %input: memref<4096xf32>,
      %output: memref<4096xf32>) {

    %c1 = arith.constant 1 : index
    %c4095 = arith.constant 4095 : index

    %two = arith.constant 2.0 : f32
    %quarter = arith.constant 0.25 : f32

    scf.for %i = %c1 to %c4095 step %c1 {
      %im1 = arith.subi %i, %c1 : index
      %ip1 = arith.addi %i, %c1 : index

      %left = memref.load %input[%im1]
          : memref<4096xf32>

      %center = memref.load %input[%i]
          : memref<4096xf32>

      %right = memref.load %input[%ip1]
          : memref<4096xf32>

      %center2 = arith.mulf %center, %two : f32
      %s0 = arith.addf %left, %center2 : f32
      %s1 = arith.addf %s0, %right : f32
      %result = arith.mulf %s1, %quarter : f32

      memref.store %result, %output[%i]
          : memref<4096xf32>
    }

    return
  }
}
