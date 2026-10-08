module {
  func.func @f(%arg0: i32) -> i32 {
    %c1_i32 = arith.constant 1 : i32
    %c2_i32 = arith.constant 2 : i32
    %c5_i32 = arith.constant 5 : i32
    %c0_i32 = arith.constant 0 : i32
    cf.br ^bb1(%c0_i32, %arg0 : i32, i32)
  ^bb1(%0: i32, %1: i32):  // 2 preds: ^bb0, ^bb6
    %2 = arith.cmpi sgt, %1, %c0_i32 : i32
    cf.cond_br %2, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %3 = arith.cmpi sgt, %1, %c5_i32 : i32
    cf.cond_br %3, ^bb4, ^bb5
  ^bb3:  // pred: ^bb1
    return %0 : i32
  ^bb4:  // pred: ^bb2
    %4 = arith.addi %0, %c2_i32 : i32
    cf.br ^bb6(%4 : i32)
  ^bb5:  // pred: ^bb2
    %5 = arith.addi %0, %c1_i32 : i32
    cf.br ^bb6(%5 : i32)
  ^bb6(%6: i32):  // 2 preds: ^bb4, ^bb5
    %7 = arith.subi %1, %c1_i32 : i32
    cf.br ^bb1(%6, %7 : i32, i32)
  }
}

