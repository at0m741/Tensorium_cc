// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: depth = 2
// CHECK: count = 1
// CHECK: vectorization = illegal
// CHECK-LABEL: mopt.loop #1
// CHECK: depth = 1
// CHECK-LABEL: mopt.loop #2
// CHECK: depth = 0

module {
  func.func @matmul(
      %A: memref<64x96xf32>,
      %B: memref<96x80xf32>,
      %C: memref<64x80xf32>) {

    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index

    %c64 = arith.constant 64 : index
    %c80 = arith.constant 80 : index
    %c96 = arith.constant 96 : index

    %zero = arith.constant 0.0 : f32

    scf.for %i = %c0 to %c64 step %c1 {
      scf.for %j = %c0 to %c80 step %c1 {

        %sum = scf.for %k = %c0 to %c96 step %c1
            iter_args(%acc = %zero) -> (f32) {

          %a = memref.load %A[%i, %k]
              : memref<64x96xf32>

          %b = memref.load %B[%k, %j]
              : memref<96x80xf32>

          %product = arith.mulf %a, %b : f32
          %next = arith.addf %acc, %product : f32

          scf.yield %next : f32
        }

        memref.store %sum, %C[%i, %j]
            : memref<64x80xf32>
      }
    }

    return
  }
}
