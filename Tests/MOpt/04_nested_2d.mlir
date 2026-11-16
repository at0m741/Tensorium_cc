// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: depth = 1
// CHECK: parent = true
// CHECK-LABEL: mopt.loop #1
// CHECK: depth = 0
// CHECK: parent = false

module {
  func.func @nested_2d(
      %A: memref<128x256xf32>,
      %B: memref<128x256xf32>,
      %C: memref<128x256xf32>) {

    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c128 = arith.constant 128 : index
    %c256 = arith.constant 256 : index

    scf.for %i = %c0 to %c128 step %c1 {
      scf.for %j = %c0 to %c256 step %c1 {
        %a = memref.load %A[%i, %j]
            : memref<128x256xf32>

        %b = memref.load %B[%i, %j]
            : memref<128x256xf32>

        %x = arith.addf %a, %b : f32

        memref.store %x, %C[%i, %j]
            : memref<128x256xf32>
      }
    }

    return
  }
}
