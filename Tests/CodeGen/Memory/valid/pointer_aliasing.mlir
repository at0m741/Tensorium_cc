module {
  func.func private @update_pair(%arg0: !tensorium.ptr<i32>, %arg1: !tensorium.ptr<i32>) {
    %c1_i32 = arith.constant 1 : i32
    tensorium.store %c1_i32, %arg0 : <i32>
    %0 = tensorium.load %arg0 : <i32>
    %c41_i32 = arith.constant 41 : i32
    %1 = arith.addi %0, %c41_i32 : i32
    tensorium.store %1, %arg1 : <i32>
    return
  }
  func.func @main() -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    tensorium.store %c0_i32_0, %1 : <i32>
    call @update_pair(%0, %1) : (!tensorium.ptr<i32>, !tensorium.ptr<i32>) -> ()
    %2 = tensorium.load %0 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %3 = arith.cmpi ne, %2, %c1_i32 : i32
    %4 = arith.extui %3 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %5 = arith.cmpi ne, %4, %c0_i32_1 : i32
    cf.cond_br %5, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %c1_i32_2 = arith.constant 1 : i32
    return %c1_i32_2 : i32
  ^bb2:  // pred: ^bb0
    %6 = tensorium.load %1 : <i32>
    %c42_i32 = arith.constant 42 : i32
    %7 = arith.cmpi ne, %6, %c42_i32 : i32
    %8 = arith.extui %7 : i1 to i32
    %c0_i32_3 = arith.constant 0 : i32
    %9 = arith.cmpi ne, %8, %c0_i32_3 : i32
    cf.cond_br %9, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %c2_i32 = arith.constant 2 : i32
    return %c2_i32 : i32
  ^bb4:  // pred: ^bb2
    call @update_pair(%0, %0) : (!tensorium.ptr<i32>, !tensorium.ptr<i32>) -> ()
    %10 = tensorium.load %0 : <i32>
    %c42_i32_4 = arith.constant 42 : i32
    %11 = arith.cmpi ne, %10, %c42_i32_4 : i32
    %12 = arith.extui %11 : i1 to i32
    %c0_i32_5 = arith.constant 0 : i32
    %13 = arith.cmpi ne, %12, %c0_i32_5 : i32
    cf.cond_br %13, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %c3_i32 = arith.constant 3 : i32
    return %c3_i32 : i32
  ^bb6:  // pred: ^bb4
    %c0_i32_6 = arith.constant 0 : i32
    return %c0_i32_6 : i32
  }
}

