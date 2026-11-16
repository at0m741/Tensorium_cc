// RUN: %mopt %s | FileCheck %s
// CHECK-LABEL: mopt.loop #0
// CHECK: loop-carried = 1
// CHECK: kind = RAW
// CHECK-NEXT: certainty = proven
// CHECK-NEXT: distance = 1
// CHECK: vector-plan:
// CHECK-NEXT: decision = rejected
// CHECK: reasons = [target-width-and-element-type, loop-carried-dependence]
// CHECK: vectorization = illegal

module {
  func.func @shifted_inplace(%a: memref<4096xf32>) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c4095 = arith.constant 4095 : index

    scf.for %i = %c0 to %c4095 step %c1 {
      %x = memref.load %a[%i] : memref<4096xf32>
      %i1 = arith.addi %i, %c1 : index
      memref.store %x, %a[%i1] : memref<4096xf32>
    }

    return
  }
}
