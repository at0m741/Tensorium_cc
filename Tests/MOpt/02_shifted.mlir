// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: trip-count = 4095
// CHECK: index[0] = 1*iv+1
// CHECK: vectorization = unknown

module {
  func.func @shifted(
      %A: memref<4096xf32>,
      %B: memref<4096xf32>) {

    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c4095 = arith.constant 4095 : index

    scf.for %i = %c0 to %c4095 step %c1 {
      %ip1 = arith.addi %i, %c1 : index

      %a0 = memref.load %A[%i] : memref<4096xf32>
      %a1 = memref.load %A[%ip1] : memref<4096xf32>

      %sum = arith.addf %a0, %a1 : f32

      memref.store %sum, %B[%i] : memref<4096xf32>
    }

    return
  }
}
