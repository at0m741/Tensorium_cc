// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: depth = 1
// CHECK: physical-stride = 1
// CHECK-LABEL: mopt.loop #1
// CHECK: depth = 0

module {
  func.func @transpose(
      %A: memref<128x256xf32>,
      %B: memref<256x128xf32>) {

    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c128 = arith.constant 128 : index
    %c256 = arith.constant 256 : index

    scf.for %i = %c0 to %c128 step %c1 {
      scf.for %j = %c0 to %c256 step %c1 {
        %x = memref.load %A[%i, %j]
            : memref<128x256xf32>

        memref.store %x, %B[%j, %i]
            : memref<256x128xf32>
      }
    }

    return
  }
}
