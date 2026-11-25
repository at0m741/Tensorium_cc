module attributes {llvm.data_layout = "e-m:o-i64:64-i128:128-n32:64-S128", llvm.target_triple = "aarch64-apple-macosx13.0.0"} {
  func.func @loop_for_storage(%arg0: i32) -> i32 {
    %c1_i32 = arith.constant 1 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    %2 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %2 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %3 = tensorium.load %2 : <i32>
    %4 = tensorium.load %1 : <i32>
    %5 = arith.cmpi slt, %3, %4 : i32
    cf.cond_br %5, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %6 = tensorium.alloca : <i32>
    %7 = tensorium.load %2 : <i32>
    %8 = arith.muli %7, %c2_i32 : i32
    tensorium.store %8, %6 : <i32>
    %9 = tensorium.load %6 : <i32>
    %10 = arith.addi %9, %c1_i32 : i32
    tensorium.store %10, %6 : <i32>
    %11 = tensorium.load %0 : <i32>
    %12 = tensorium.load %6 : <i32>
    %13 = arith.addi %11, %12 : i32
    tensorium.store %13, %0 : <i32>
    %14 = tensorium.load %2 : <i32>
    %15 = arith.addi %14, %c1_i32 : i32
    tensorium.store %15, %2 : <i32>
    cf.br ^bb1
  ^bb3:  // pred: ^bb1
    %16 = tensorium.load %0 : <i32>
    return %16 : i32
  }
  func.func @loop_while_storage(%arg0: i32) -> i32 {
    %c1_i32 = arith.constant 1 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %2 = tensorium.load %1 : <i32>
    %3 = arith.cmpi sgt, %2, %c0_i32 : i32
    cf.cond_br %3, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %4 = tensorium.alloca : <i32>
    %5 = tensorium.load %1 : <i32>
    %6 = arith.muli %5, %c2_i32 : i32
    tensorium.store %6, %4 : <i32>
    %7 = tensorium.load %4 : <i32>
    %8 = arith.addi %7, %c1_i32 : i32
    tensorium.store %8, %4 : <i32>
    %9 = tensorium.load %0 : <i32>
    %10 = tensorium.load %4 : <i32>
    %11 = arith.addi %9, %10 : i32
    tensorium.store %11, %0 : <i32>
    %12 = tensorium.load %1 : <i32>
    %13 = arith.subi %12, %c1_i32 : i32
    tensorium.store %13, %1 : <i32>
    cf.br ^bb1
  ^bb3:  // pred: ^bb1
    %14 = tensorium.load %0 : <i32>
    return %14 : i32
  }
  func.func @loop_address_storage(%arg0: i32) -> i32 {
    %c1_i32 = arith.constant 1 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    %2 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %2 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %3 = tensorium.load %2 : <i32>
    %4 = tensorium.load %1 : <i32>
    %5 = arith.cmpi slt, %3, %4 : i32
    cf.cond_br %5, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %6 = tensorium.alloca : <i32>
    %7 = tensorium.load %2 : <i32>
    %8 = arith.muli %7, %c2_i32 : i32
    tensorium.store %8, %6 : <i32>
    %9 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %6, %9 : <!tensorium.ptr<i32>>
    %10 = tensorium.load %9 : <!tensorium.ptr<i32>>
    %11 = tensorium.load %10 : <i32>
    %12 = arith.addi %11, %c1_i32 : i32
    tensorium.store %12, %10 : <i32>
    %13 = tensorium.load %0 : <i32>
    %14 = tensorium.load %6 : <i32>
    %15 = arith.addi %13, %14 : i32
    tensorium.store %15, %0 : <i32>
    %16 = tensorium.load %2 : <i32>
    %17 = arith.addi %16, %c1_i32 : i32
    tensorium.store %17, %2 : <i32>
    cf.br ^bb1
  ^bb3:  // pred: ^bb1
    %18 = tensorium.load %0 : <i32>
    return %18 : i32
  }
  func.func @loop_array_storage(%arg0: i32) -> i32 {
    %c1_i64 = arith.constant 1 : i64
    %c0_i64 = arith.constant 0 : i64
    %c3_i32 = arith.constant 3 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    %2 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %2 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %3 = tensorium.load %2 : <i32>
    %4 = tensorium.load %1 : <i32>
    %5 = arith.cmpi slt, %3, %4 : i32
    cf.cond_br %5, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %6 = tensorium.alloca : <!tensorium.array<2, i32>>
    %7 = tensorium.array_decay %6 : <!tensorium.array<2, i32>> -> <i32>
    %8 = tensorium.ptr_offset %7[%c0_i64] : <i32>, i64
    %9 = tensorium.load %2 : <i32>
    tensorium.store %9, %8 : <i32>
    %10 = tensorium.array_decay %6 : <!tensorium.array<2, i32>> -> <i32>
    %11 = tensorium.ptr_offset %10[%c1_i64] : <i32>, i64
    %12 = tensorium.array_decay %6 : <!tensorium.array<2, i32>> -> <i32>
    %13 = tensorium.ptr_offset %12[%c0_i64] : <i32>, i64
    %14 = tensorium.load %13 : <i32>
    %15 = arith.addi %14, %c3_i32 : i32
    tensorium.store %15, %11 : <i32>
    %16 = tensorium.load %0 : <i32>
    %17 = tensorium.array_decay %6 : <!tensorium.array<2, i32>> -> <i32>
    %18 = tensorium.ptr_offset %17[%c0_i64] : <i32>, i64
    %19 = tensorium.load %18 : <i32>
    %20 = tensorium.array_decay %6 : <!tensorium.array<2, i32>> -> <i32>
    %21 = tensorium.ptr_offset %20[%c1_i64] : <i32>, i64
    %22 = tensorium.load %21 : <i32>
    %23 = arith.addi %19, %22 : i32
    %24 = arith.addi %16, %23 : i32
    tensorium.store %24, %0 : <i32>
    %25 = tensorium.load %2 : <i32>
    %26 = arith.addi %25, %c1_i32 : i32
    tensorium.store %26, %2 : <i32>
    cf.br ^bb1
  ^bb3:  // pred: ^bb1
    %27 = tensorium.load %0 : <i32>
    return %27 : i32
  }
  func.func @loop_nested_storage(%arg0: i32) -> i32 {
    %c2_i32 = arith.constant 2 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    %2 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %2 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb6
    %3 = tensorium.load %2 : <i32>
    %4 = tensorium.load %1 : <i32>
    %5 = arith.cmpi slt, %3, %4 : i32
    cf.cond_br %5, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %6 = tensorium.alloca : <i32>
    %7 = tensorium.load %2 : <i32>
    tensorium.store %7, %6 : <i32>
    %8 = tensorium.load %6 : <i32>
    %9 = arith.addi %8, %c1_i32 : i32
    tensorium.store %9, %6 : <i32>
    %10 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %10 : <i32>
    cf.br ^bb4
  ^bb3:  // pred: ^bb1
    %11 = tensorium.load %0 : <i32>
    return %11 : i32
  ^bb4:  // 2 preds: ^bb2, ^bb5
    %12 = tensorium.load %10 : <i32>
    %13 = arith.cmpi slt, %12, %c2_i32 : i32
    cf.cond_br %13, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %14 = tensorium.alloca : <i32>
    %15 = tensorium.load %2 : <i32>
    %16 = tensorium.load %10 : <i32>
    %17 = arith.addi %15, %16 : i32
    tensorium.store %17, %14 : <i32>
    %18 = tensorium.load %14 : <i32>
    %19 = arith.addi %18, %c2_i32 : i32
    tensorium.store %19, %14 : <i32>
    %20 = tensorium.load %0 : <i32>
    %21 = tensorium.load %14 : <i32>
    %22 = arith.addi %20, %21 : i32
    tensorium.store %22, %0 : <i32>
    %23 = tensorium.load %10 : <i32>
    %24 = arith.addi %23, %c1_i32 : i32
    tensorium.store %24, %10 : <i32>
    cf.br ^bb4
  ^bb6:  // pred: ^bb4
    %25 = tensorium.load %0 : <i32>
    %26 = tensorium.load %6 : <i32>
    %27 = arith.addi %25, %26 : i32
    tensorium.store %27, %0 : <i32>
    %28 = tensorium.load %2 : <i32>
    %29 = arith.addi %28, %c1_i32 : i32
    tensorium.store %29, %2 : <i32>
    cf.br ^bb1
  }
  func.func private @loop_next(%arg0: !tensorium.ptr<i32>, %arg1: i32) -> i32 {
    %c1_i32 = arith.constant 1 : i32
    %0 = tensorium.load %arg0 : <i32>
    %1 = arith.addi %0, %c1_i32 : i32
    tensorium.store %1, %arg0 : <i32>
    %2 = tensorium.load %arg0 : <i32>
    %3 = arith.addi %arg1, %2 : i32
    return %3 : i32
  }
  func.func @loop_call_initializer(%arg0: i32, %arg1: !tensorium.ptr<i32>) -> i32 {
    %c1_i32 = arith.constant 1 : i32
    %c2_i32 = arith.constant 2 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %arg1, %1 : <!tensorium.ptr<i32>>
    %2 = tensorium.alloca : <i32>
    tensorium.store %arg0, %2 : <i32>
    %3 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %3 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %4 = tensorium.load %3 : <i32>
    %5 = tensorium.load %2 : <i32>
    %6 = arith.cmpi slt, %4, %5 : i32
    cf.cond_br %6, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %7 = tensorium.alloca : <i32>
    %8 = tensorium.load %1 : <!tensorium.ptr<i32>>
    %9 = tensorium.load %3 : <i32>
    %10 = arith.muli %9, %c2_i32 : i32
    %11 = call @loop_next(%8, %10) : (!tensorium.ptr<i32>, i32) -> i32
    tensorium.store %11, %7 : <i32>
    %12 = tensorium.load %7 : <i32>
    %13 = arith.addi %12, %c1_i32 : i32
    tensorium.store %13, %7 : <i32>
    %14 = tensorium.load %0 : <i32>
    %15 = tensorium.load %7 : <i32>
    %16 = arith.addi %14, %15 : i32
    tensorium.store %16, %0 : <i32>
    %17 = tensorium.load %3 : <i32>
    %18 = arith.addi %17, %c1_i32 : i32
    tensorium.store %18, %3 : <i32>
    cf.br ^bb1
  ^bb3:  // pred: ^bb1
    %19 = tensorium.load %0 : <i32>
    return %19 : i32
  }
  func.func @loop_conditional_initializer(%arg0: i32, %arg1: !tensorium.ptr<i32>) -> i32 {
    %c2_i32 = arith.constant 2 : i32
    %c4_i32 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %arg1, %1 : <!tensorium.ptr<i32>>
    %2 = tensorium.alloca : <i32>
    tensorium.store %arg0, %2 : <i32>
    %3 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %3 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %4 = tensorium.load %3 : <i32>
    %5 = tensorium.load %2 : <i32>
    %6 = arith.cmpi slt, %4, %5 : i32
    cf.cond_br %6, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %7 = tensorium.load %3 : <i32>
    %8 = arith.cmpi eq, %7, %c1_i32 : i32
    cf.cond_br %8, ^bb3, ^bb5
  ^bb3:  // 2 preds: ^bb2, ^bb6
    %9 = tensorium.load %3 : <i32>
    %10 = arith.addi %9, %c1_i32 : i32
    tensorium.store %10, %3 : <i32>
    cf.br ^bb1
  ^bb4:  // 2 preds: ^bb1, ^bb5
    %11 = tensorium.load %0 : <i32>
    return %11 : i32
  ^bb5:  // pred: ^bb2
    %12 = tensorium.load %3 : <i32>
    %13 = arith.cmpi eq, %12, %c4_i32 : i32
    cf.cond_br %13, ^bb4, ^bb6
  ^bb6:  // pred: ^bb5
    %14 = tensorium.alloca : <i32>
    %15 = tensorium.load %1 : <!tensorium.ptr<i32>>
    %16 = tensorium.load %3 : <i32>
    %17 = arith.muli %16, %c2_i32 : i32
    %18 = call @loop_next(%15, %17) : (!tensorium.ptr<i32>, i32) -> i32
    tensorium.store %18, %14 : <i32>
    %19 = tensorium.load %14 : <i32>
    %20 = arith.addi %19, %c1_i32 : i32
    tensorium.store %20, %14 : <i32>
    %21 = tensorium.load %0 : <i32>
    %22 = tensorium.load %14 : <i32>
    %23 = arith.addi %21, %22 : i32
    tensorium.store %23, %0 : <i32>
    cf.br ^bb3
  }
  func.func @loop_load_initializer(%arg0: i32, %arg1: !tensorium.ptr<i32>) -> i32 {
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %arg1, %1 : <!tensorium.ptr<i32>>
    %2 = tensorium.alloca : <i32>
    tensorium.store %arg0, %2 : <i32>
    %3 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %3 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %4 = tensorium.load %3 : <i32>
    %5 = tensorium.load %2 : <i32>
    %6 = arith.cmpi slt, %4, %5 : i32
    cf.cond_br %6, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %7 = tensorium.alloca : <i32>
    %8 = tensorium.load %1 : <!tensorium.ptr<i32>>
    %9 = tensorium.load %8 : <i32>
    tensorium.store %9, %7 : <i32>
    %10 = tensorium.load %7 : <i32>
    %11 = arith.addi %10, %c1_i32 : i32
    tensorium.store %11, %7 : <i32>
    %12 = tensorium.load %0 : <i32>
    %13 = tensorium.load %7 : <i32>
    %14 = arith.addi %12, %13 : i32
    tensorium.store %14, %0 : <i32>
    %15 = tensorium.load %3 : <i32>
    %16 = arith.addi %15, %c1_i32 : i32
    tensorium.store %16, %3 : <i32>
    cf.br ^bb1
  ^bb3:  // pred: ^bb1
    %17 = tensorium.load %0 : <i32>
    return %17 : i32
  }
  func.func @main() -> i32 {
    %c23_i32 = arith.constant 23 : i32
    %c22_i32 = arith.constant 22 : i32
    %c21_i32 = arith.constant 21 : i32
    %c20_i32 = arith.constant 20 : i32
    %c19_i32 = arith.constant 19 : i32
    %c18_i32 = arith.constant 18 : i32
    %c17_i32 = arith.constant 17 : i32
    %c14_i32 = arith.constant 14 : i32
    %c15_i32 = arith.constant 15 : i32
    %c13_i32 = arith.constant 13 : i32
    %c12_i32 = arith.constant 12 : i32
    %c11_i32 = arith.constant 11 : i32
    %c27_i32 = arith.constant 27 : i32
    %c10_i32 = arith.constant 10 : i32
    %c9_i32 = arith.constant 9 : i32
    %c8_i32 = arith.constant 8 : i32
    %c7_i32 = arith.constant 7 : i32
    %c6_i32 = arith.constant 6 : i32
    %c5_i32 = arith.constant 5 : i32
    %c24_i32 = arith.constant 24 : i32
    %c3_i32 = arith.constant 3 : i32
    %c1000000_i32 = arith.constant 1000000 : i32
    %c1000_i32 = arith.constant 1000 : i32
    %c2_i32 = arith.constant 2 : i32
    %c16_i32 = arith.constant 16 : i32
    %c4_i32 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = call @loop_for_storage(%c0_i32) : (i32) -> i32
    %1 = arith.cmpi ne, %0, %c0_i32 : i32
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    return %c1_i32 : i32
  ^bb2:  // pred: ^bb0
    %2 = call @loop_for_storage(%c4_i32) : (i32) -> i32
    %3 = arith.cmpi ne, %2, %c16_i32 : i32
    cf.cond_br %3, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    return %c2_i32 : i32
  ^bb4:  // pred: ^bb2
    %4 = call @loop_for_storage(%c1000_i32) : (i32) -> i32
    %5 = arith.cmpi ne, %4, %c1000000_i32 : i32
    cf.cond_br %5, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    return %c3_i32 : i32
  ^bb6:  // pred: ^bb4
    %6 = call @loop_while_storage(%c0_i32) : (i32) -> i32
    %7 = arith.cmpi ne, %6, %c0_i32 : i32
    cf.cond_br %7, ^bb7, ^bb8
  ^bb7:  // pred: ^bb6
    return %c4_i32 : i32
  ^bb8:  // pred: ^bb6
    %8 = call @loop_while_storage(%c4_i32) : (i32) -> i32
    %9 = arith.cmpi ne, %8, %c24_i32 : i32
    cf.cond_br %9, ^bb9, ^bb10
  ^bb9:  // pred: ^bb8
    return %c5_i32 : i32
  ^bb10:  // pred: ^bb8
    %10 = call @loop_address_storage(%c0_i32) : (i32) -> i32
    %11 = arith.cmpi ne, %10, %c0_i32 : i32
    cf.cond_br %11, ^bb11, ^bb12
  ^bb11:  // pred: ^bb10
    return %c6_i32 : i32
  ^bb12:  // pred: ^bb10
    %12 = call @loop_address_storage(%c4_i32) : (i32) -> i32
    %13 = arith.cmpi ne, %12, %c16_i32 : i32
    cf.cond_br %13, ^bb13, ^bb14
  ^bb13:  // pred: ^bb12
    return %c7_i32 : i32
  ^bb14:  // pred: ^bb12
    %14 = call @loop_array_storage(%c0_i32) : (i32) -> i32
    %15 = arith.cmpi ne, %14, %c0_i32 : i32
    cf.cond_br %15, ^bb15, ^bb16
  ^bb15:  // pred: ^bb14
    return %c8_i32 : i32
  ^bb16:  // pred: ^bb14
    %16 = call @loop_array_storage(%c4_i32) : (i32) -> i32
    %17 = arith.cmpi ne, %16, %c24_i32 : i32
    cf.cond_br %17, ^bb17, ^bb18
  ^bb17:  // pred: ^bb16
    return %c9_i32 : i32
  ^bb18:  // pred: ^bb16
    %18 = call @loop_nested_storage(%c0_i32) : (i32) -> i32
    %19 = arith.cmpi ne, %18, %c0_i32 : i32
    cf.cond_br %19, ^bb19, ^bb20
  ^bb19:  // pred: ^bb18
    return %c10_i32 : i32
  ^bb20:  // pred: ^bb18
    %20 = call @loop_nested_storage(%c3_i32) : (i32) -> i32
    %21 = arith.cmpi ne, %20, %c27_i32 : i32
    cf.cond_br %21, ^bb21, ^bb22
  ^bb21:  // pred: ^bb20
    return %c11_i32 : i32
  ^bb22:  // pred: ^bb20
    %22 = tensorium.alloca : <i32>
    tensorium.store %c0_i32, %22 : <i32>
    %23 = call @loop_call_initializer(%c0_i32, %22) : (i32, !tensorium.ptr<i32>) -> i32
    %24 = arith.cmpi ne, %23, %c0_i32 : i32
    cf.cond_br %24, ^bb23, ^bb24
  ^bb23:  // pred: ^bb22
    return %c12_i32 : i32
  ^bb24:  // pred: ^bb22
    %25 = tensorium.load %22 : <i32>
    %26 = arith.cmpi ne, %25, %c0_i32 : i32
    cf.cond_br %26, ^bb25, ^bb26
  ^bb25:  // pred: ^bb24
    return %c13_i32 : i32
  ^bb26:  // pred: ^bb24
    %27 = call @loop_call_initializer(%c3_i32, %22) : (i32, !tensorium.ptr<i32>) -> i32
    %28 = arith.cmpi ne, %27, %c15_i32 : i32
    cf.cond_br %28, ^bb27, ^bb28
  ^bb27:  // pred: ^bb26
    return %c14_i32 : i32
  ^bb28:  // pred: ^bb26
    %29 = tensorium.load %22 : <i32>
    %30 = arith.cmpi ne, %29, %c3_i32 : i32
    cf.cond_br %30, ^bb29, ^bb30
  ^bb29:  // pred: ^bb28
    return %c15_i32 : i32
  ^bb30:  // pred: ^bb28
    tensorium.store %c0_i32, %22 : <i32>
    %31 = call @loop_conditional_initializer(%c0_i32, %22) : (i32, !tensorium.ptr<i32>) -> i32
    %32 = arith.cmpi ne, %31, %c0_i32 : i32
    cf.cond_br %32, ^bb31, ^bb32
  ^bb31:  // pred: ^bb30
    return %c16_i32 : i32
  ^bb32:  // pred: ^bb30
    %33 = tensorium.load %22 : <i32>
    %34 = arith.cmpi ne, %33, %c0_i32 : i32
    cf.cond_br %34, ^bb33, ^bb34
  ^bb33:  // pred: ^bb32
    return %c17_i32 : i32
  ^bb34:  // pred: ^bb32
    %35 = call @loop_conditional_initializer(%c3_i32, %22) : (i32, !tensorium.ptr<i32>) -> i32
    %36 = arith.cmpi ne, %35, %c9_i32 : i32
    cf.cond_br %36, ^bb35, ^bb36
  ^bb35:  // pred: ^bb34
    return %c18_i32 : i32
  ^bb36:  // pred: ^bb34
    %37 = tensorium.load %22 : <i32>
    %38 = arith.cmpi ne, %37, %c2_i32 : i32
    cf.cond_br %38, ^bb37, ^bb38
  ^bb37:  // pred: ^bb36
    return %c19_i32 : i32
  ^bb38:  // pred: ^bb36
    tensorium.store %c0_i32, %22 : <i32>
    %39 = call @loop_conditional_initializer(%c7_i32, %22) : (i32, !tensorium.ptr<i32>) -> i32
    %40 = arith.cmpi ne, %39, %c19_i32 : i32
    cf.cond_br %40, ^bb39, ^bb40
  ^bb39:  // pred: ^bb38
    return %c20_i32 : i32
  ^bb40:  // pred: ^bb38
    %41 = tensorium.load %22 : <i32>
    %42 = arith.cmpi ne, %41, %c3_i32 : i32
    cf.cond_br %42, ^bb41, ^bb42
  ^bb41:  // pred: ^bb40
    return %c21_i32 : i32
  ^bb42:  // pred: ^bb40
    %43 = tensorium.null : <i32>
    %44 = call @loop_load_initializer(%c0_i32, %43) : (i32, !tensorium.ptr<i32>) -> i32
    %45 = arith.cmpi ne, %44, %c0_i32 : i32
    cf.cond_br %45, ^bb43, ^bb44
  ^bb43:  // pred: ^bb42
    return %c22_i32 : i32
  ^bb44:  // pred: ^bb42
    %46 = tensorium.alloca : <i32>
    tensorium.store %c3_i32, %46 : <i32>
    %47 = call @loop_load_initializer(%c3_i32, %46) : (i32, !tensorium.ptr<i32>) -> i32
    %48 = arith.cmpi ne, %47, %c12_i32 : i32
    cf.cond_br %48, ^bb45, ^bb46
  ^bb45:  // pred: ^bb44
    return %c23_i32 : i32
  ^bb46:  // pred: ^bb44
    %49 = tensorium.load %46 : <i32>
    %50 = arith.cmpi ne, %49, %c3_i32 : i32
    cf.cond_br %50, ^bb47, ^bb48
  ^bb47:  // pred: ^bb46
    return %c24_i32 : i32
  ^bb48:  // pred: ^bb46
    return %c0_i32 : i32
  }
}

