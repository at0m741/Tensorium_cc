module {
  func.func @f(%arg0: i32) -> i32 {
    %alloca = memref.alloca() : memref<i32>
    %c0_i32 = arith.constant 0 : i32
    memref.store %c0_i32, %alloca[] : memref<i32>
    %alloca_0 = memref.alloca() : memref<i32>
    memref.store %arg0, %alloca_0[] : memref<i32>
    cf.br ^bb1
  ^bb1:  // 3 preds: ^bb0, ^bb4, ^bb7
    %0 = memref.load %alloca_0[] : memref<i32>
    %c0_i32_1 = arith.constant 0 : i32
    %1 = arith.cmpi sgt, %0, %c0_i32_1 : i32
    %2 = arith.extui %1 : i1 to i32
    %c0_i32_2 = arith.constant 0 : i32
    %3 = arith.cmpi ne, %2, %c0_i32_2 : i32
    cf.cond_br %3, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %4 = memref.load %alloca_0[] : memref<i32>
    %c1_i32 = arith.constant 1 : i32
    %5 = arith.subi %4, %c1_i32 : i32
    memref.store %5, %alloca_0[] : memref<i32>
    %6 = memref.load %alloca_0[] : memref<i32>
    %c3_i32 = arith.constant 3 : i32
    %7 = arith.cmpi eq, %6, %c3_i32 : i32
    %8 = arith.extui %7 : i1 to i32
    %c0_i32_3 = arith.constant 0 : i32
    %9 = arith.cmpi ne, %8, %c0_i32_3 : i32
    cf.cond_br %9, ^bb4, ^bb5
  ^bb3:  // 2 preds: ^bb1, ^bb6
    %10 = memref.load %alloca[] : memref<i32>
    return %10 : i32
  ^bb4:  // pred: ^bb2
    cf.br ^bb1
  ^bb5:  // pred: ^bb2
    %11 = memref.load %alloca_0[] : memref<i32>
    %c1_i32_4 = arith.constant 1 : i32
    %12 = arith.cmpi eq, %11, %c1_i32_4 : i32
    %13 = arith.extui %12 : i1 to i32
    %c0_i32_5 = arith.constant 0 : i32
    %14 = arith.cmpi ne, %13, %c0_i32_5 : i32
    cf.cond_br %14, ^bb6, ^bb7
  ^bb6:  // pred: ^bb5
    cf.br ^bb3
  ^bb7:  // pred: ^bb5
    %15 = memref.load %alloca[] : memref<i32>
    %c1_i32_6 = arith.constant 1 : i32
    %16 = arith.addi %15, %c1_i32_6 : i32
    memref.store %16, %alloca[] : memref<i32>
    cf.br ^bb1
  }
}

