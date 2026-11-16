// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: upper = dynamic
// CHECK-NEXT: step = 1
// CHECK-NEXT: trip-count = dynamic

module {
  func.func @dynamic(
      %A: memref<?xf32>,
      %B: memref<?xf32>,
      %C: memref<?xf32>,
      %N: index) {

    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index

    scf.for %i = %c0 to %N step %c1 {
      %a = memref.load %A[%i] : memref<?xf32>
      %b = memref.load %B[%i] : memref<?xf32>

      %x = arith.addf %a, %b : f32

      memref.store %x, %C[%i] : memref<?xf32>
    }

    return
  }
}
