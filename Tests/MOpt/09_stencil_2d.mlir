// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: depth = 1
// CHECK-LABEL: mopt.loop #1
// CHECK: depth = 0

module {
  func.func @stencil_2d(
      %input: memref<64x64xf32>,
      %output: memref<64x64xf32>) {

    %c1 = arith.constant 1 : index
    %c63 = arith.constant 63 : index

    %scale = arith.constant 0.2 : f32

    scf.for %i = %c1 to %c63 step %c1 {
      %im1 = arith.subi %i, %c1 : index
      %ip1 = arith.addi %i, %c1 : index

      scf.for %j = %c1 to %c63 step %c1 {
        %jm1 = arith.subi %j, %c1 : index
        %jp1 = arith.addi %j, %c1 : index

        %center = memref.load %input[%i, %j]
            : memref<64x64xf32>

        %up = memref.load %input[%im1, %j]
            : memref<64x64xf32>

        %down = memref.load %input[%ip1, %j]
            : memref<64x64xf32>

        %left = memref.load %input[%i, %jm1]
            : memref<64x64xf32>

        %right = memref.load %input[%i, %jp1]
            : memref<64x64xf32>

        %s0 = arith.addf %center, %up : f32
        %s1 = arith.addf %s0, %down : f32
        %s2 = arith.addf %s1, %left : f32
        %s3 = arith.addf %s2, %right : f32

        %result = arith.mulf %s3, %scale : f32

        memref.store %result, %output[%i, %j]
            : memref<64x64xf32>
      }
    }

    return
  }
}
