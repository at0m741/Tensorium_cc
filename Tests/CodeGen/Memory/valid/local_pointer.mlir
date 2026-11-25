module attributes {llvm.data_layout = "e-m:o-i64:64-i128:128-n32:64-S128", llvm.target_triple = "aarch64-apple-macosx13.0.0"} {
  func.func @main() -> i32 {
    %0 = tensorium.alloca : <i32>
    %c1_i32 = arith.constant 1 : i32
    tensorium.store %c1_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    %c7_i32 = arith.constant 7 : i32
    tensorium.store %c7_i32, %1 : <i32>
    %2 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %0, %2 : <!tensorium.ptr<i32>>
    %3 = tensorium.load %2 : <!tensorium.ptr<i32>>
    %c42_i32 = arith.constant 42 : i32
    tensorium.store %c42_i32, %3 : <i32>
    %4 = tensorium.load %0 : <i32>
    %c42_i32_0 = arith.constant 42 : i32
    %5 = arith.cmpi ne, %4, %c42_i32_0 : i32
    %6 = arith.extui %5 : i1 to i32
    %c0_i32 = arith.constant 0 : i32
    %7 = arith.cmpi ne, %6, %c0_i32 : i32
    cf.cond_br %7, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %c1_i32_1 = arith.constant 1 : i32
    return %c1_i32_1 : i32
  ^bb2:  // pred: ^bb0
    %8 = tensorium.load %2 : <!tensorium.ptr<i32>>
    tensorium.store %1, %2 : <!tensorium.ptr<i32>>
    %9 = tensorium.load %2 : <!tensorium.ptr<i32>>
    %c9_i32 = arith.constant 9 : i32
    tensorium.store %c9_i32, %9 : <i32>
    %10 = tensorium.load %0 : <i32>
    %11 = tensorium.load %1 : <i32>
    %12 = arith.addi %10, %11 : i32
    %c51_i32 = arith.constant 51 : i32
    %13 = arith.cmpi ne, %12, %c51_i32 : i32
    %14 = arith.extui %13 : i1 to i32
    %c0_i32_2 = arith.constant 0 : i32
    %15 = arith.cmpi ne, %14, %c0_i32_2 : i32
    cf.cond_br %15, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %c2_i32 = arith.constant 2 : i32
    return %c2_i32 : i32
  ^bb4:  // pred: ^bb2
    %16 = tensorium.alloca : <!tensorium.ptr<i32>>
    %17 = tensorium.load %2 : <!tensorium.ptr<i32>>
    tensorium.store %17, %16 : <!tensorium.ptr<i32>>
    %18 = tensorium.load %16 : <!tensorium.ptr<i32>>
    %19 = tensorium.load %16 : <!tensorium.ptr<i32>>
    %20 = tensorium.load %19 : <i32>
    %c1_i32_3 = arith.constant 1 : i32
    %21 = arith.addi %20, %c1_i32_3 : i32
    tensorium.store %21, %18 : <i32>
    %22 = tensorium.load %1 : <i32>
    %c10_i32 = arith.constant 10 : i32
    %23 = arith.cmpi ne, %22, %c10_i32 : i32
    %24 = arith.extui %23 : i1 to i32
    %c0_i32_4 = arith.constant 0 : i32
    %25 = arith.cmpi ne, %24, %c0_i32_4 : i32
    cf.cond_br %25, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %c3_i32 = arith.constant 3 : i32
    return %c3_i32 : i32
  ^bb6:  // pred: ^bb4
    %c0_i32_5 = arith.constant 0 : i32
    return %c0_i32_5 : i32
  }
}

