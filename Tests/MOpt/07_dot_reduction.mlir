// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: reductions:
// CHECK-NEXT: count = 1
// CHECK: floating-point = true
// CHECK-NEXT: reassociation = false
// CHECK: vector-plan:
// CHECK-NEXT: decision = rejected
// CHECK: reasons = [target-width-and-element-type, floating-point-reduction-requires-reassociation]
// CHECK: vectorization = illegal

module {
  func.func @dot(
      %A: memref<8192xf32>,
      %B: memref<8192xf32>) -> f32 {

    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c8192 = arith.constant 8192 : index

    %zero = arith.constant 0.0 : f32

    %result = scf.for %i = %c0 to %c8192 step %c1
        iter_args(%acc = %zero) -> (f32) {

      %a = memref.load %A[%i] : memref<8192xf32>
      %b = memref.load %B[%i] : memref<8192xf32>

      %product = arith.mulf %a, %b : f32
      %next = arith.addf %acc, %product : f32

      scf.yield %next : f32
    }

    return %result : f32
  }
}
