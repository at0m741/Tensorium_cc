module {
  func.func private @set(%arg0: !tensorium.ptr<i32>, %arg1: i32) {
    tensorium.store %arg1, %arg0 : <i32>
    return
  }
  func.func @main() -> i32 {
    %0 = tensorium.alloca : <i32>
    %c1_i32 = arith.constant 1 : i32
    tensorium.store %c1_i32, %0 : <i32>
    %c42_i32 = arith.constant 42 : i32
    call @set(%0, %c42_i32) : (!tensorium.ptr<i32>, i32) -> ()
    %1 = tensorium.load %0 : <i32>
    %c42_i32_0 = arith.constant 42 : i32
    %2 = arith.cmpi ne, %1, %c42_i32_0 : i32
    %3 = arith.extui %2 : i1 to i32
    %c0_i32 = arith.constant 0 : i32
    %4 = arith.cmpi ne, %3, %c0_i32 : i32
    cf.cond_br %4, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %c1_i32_1 = arith.constant 1 : i32
    return %c1_i32_1 : i32
  ^bb2:  // pred: ^bb0
    %5 = tensorium.load %0 : <i32>
    %c42_i32_2 = arith.constant 42 : i32
    %6 = arith.cmpi ne, %5, %c42_i32_2 : i32
    %7 = arith.extui %6 : i1 to i32
    %c0_i32_3 = arith.constant 0 : i32
    %8 = arith.cmpi ne, %7, %c0_i32_3 : i32
    cf.cond_br %8, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %c2_i32 = arith.constant 2 : i32
    return %c2_i32 : i32
  ^bb4:  // pred: ^bb2
    %c11_i32 = arith.constant 11 : i32
    call @set(%0, %c11_i32) : (!tensorium.ptr<i32>, i32) -> ()
    %9 = tensorium.load %0 : <i32>
    %c11_i32_4 = arith.constant 11 : i32
    %10 = arith.cmpi ne, %9, %c11_i32_4 : i32
    %11 = arith.extui %10 : i1 to i32
    %c0_i32_5 = arith.constant 0 : i32
    %12 = arith.cmpi ne, %11, %c0_i32_5 : i32
    cf.cond_br %12, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %c3_i32 = arith.constant 3 : i32
    return %c3_i32 : i32
  ^bb6:  // pred: ^bb4
    %c0_i32_6 = arith.constant 0 : i32
    return %c0_i32_6 : i32
  }
}

