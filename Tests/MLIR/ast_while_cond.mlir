module {
  func.func @f(%arg0: i32) -> i32 {
    %alloca = memref.alloca() : memref<i32>
    %c0_i32 = arith.constant 0 : i32
    memref.store %c0_i32, %alloca[] : memref<i32>
    %alloca_0 = memref.alloca() : memref<i32>
    memref.store %arg0, %alloca_0[] : memref<i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb6
    %0 = memref.load %alloca_0[] : memref<i32>
    %c0_i32_1 = arith.constant 0 : i32
    %1 = arith.cmpi sgt, %0, %c0_i32_1 : i32
    %2 = arith.extui %1 : i1 to i32
    %c0_i32_2 = arith.constant 0 : i32
    %3 = arith.cmpi ne, %2, %c0_i32_2 : i32
    cf.cond_br %3, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %4 = memref.load %alloca_0[] : memref<i32>
    %c5_i32 = arith.constant 5 : i32
    %5 = arith.cmpi sgt, %4, %c5_i32 : i32
    %6 = arith.extui %5 : i1 to i32
    %c0_i32_3 = arith.constant 0 : i32
    %7 = arith.cmpi ne, %6, %c0_i32_3 : i32
    cf.cond_br %7, ^bb4, ^bb5
  ^bb3:  // pred: ^bb1
    %8 = memref.load %alloca[] : memref<i32>
    return %8 : i32
  ^bb4:  // pred: ^bb2
    %9 = memref.load %alloca[] : memref<i32>
    %c2_i32 = arith.constant 2 : i32
    %10 = arith.addi %9, %c2_i32 : i32
    memref.store %10, %alloca[] : memref<i32>
    cf.br ^bb6
  ^bb5:  // pred: ^bb2
    %11 = memref.load %alloca[] : memref<i32>
    %c1_i32 = arith.constant 1 : i32
    %12 = arith.addi %11, %c1_i32 : i32
    memref.store %12, %alloca[] : memref<i32>
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    %13 = memref.load %alloca_0[] : memref<i32>
    %c1_i32_4 = arith.constant 1 : i32
    %14 = arith.subi %13, %c1_i32_4 : i32
    memref.store %14, %alloca_0[] : memref<i32>
    cf.br ^bb1
  }
}

