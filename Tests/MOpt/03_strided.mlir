// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: physical-stride = 2
// CHECK-NEXT: unit-stride = false

module {
  func.func @strided(
      %A: memref<4096xf32>,
      %B: memref<2048xf32>,
      %C: memref<2048xf32>) {

    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c2 = arith.constant 2 : index
    %c2048 = arith.constant 2048 : index

    scf.for %i = %c0 to %c2048 step %c1 {
      %i2 = arith.muli %i, %c2 : index

      %a = memref.load %A[%i2] : memref<4096xf32>
      %b = memref.load %B[%i] : memref<2048xf32>

      %x = arith.addf %a, %b : f32

      memref.store %x, %C[%i] : memref<2048xf32>
    }

    return
  }
}
