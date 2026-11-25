module attributes {llvm.data_layout = "e-m:o-i64:64-i128:128-n32:64-S128", llvm.target_triple = "aarch64-apple-macosx13.0.0"} {
  func.func @loop_for_storage(%arg0: i32) -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    %2 = tensorium.alloca : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    tensorium.store %c0_i32_0, %2 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %3 = tensorium.load %2 : <i32>
    %4 = tensorium.load %1 : <i32>
    %5 = arith.cmpi slt, %3, %4 : i32
    %6 = arith.extui %5 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %7 = arith.cmpi ne, %6, %c0_i32_1 : i32
    cf.cond_br %7, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %8 = tensorium.alloca : <i32>
    %9 = tensorium.load %2 : <i32>
    %c2_i32 = arith.constant 2 : i32
    %10 = arith.muli %9, %c2_i32 : i32
    tensorium.store %10, %8 : <i32>
    %11 = tensorium.load %8 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %12 = arith.addi %11, %c1_i32 : i32
    tensorium.store %12, %8 : <i32>
    %13 = tensorium.load %0 : <i32>
    %14 = tensorium.load %8 : <i32>
    %15 = arith.addi %13, %14 : i32
    tensorium.store %15, %0 : <i32>
    cf.br ^bb3
  ^bb3:  // pred: ^bb2
    %16 = tensorium.load %2 : <i32>
    %c1_i32_2 = arith.constant 1 : i32
    %17 = arith.addi %16, %c1_i32_2 : i32
    tensorium.store %17, %2 : <i32>
    cf.br ^bb1
  ^bb4:  // pred: ^bb1
    %18 = tensorium.load %0 : <i32>
    return %18 : i32
  }
  func.func @loop_while_storage(%arg0: i32) -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %2 = tensorium.load %1 : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    %3 = arith.cmpi sgt, %2, %c0_i32_0 : i32
    %4 = arith.extui %3 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %5 = arith.cmpi ne, %4, %c0_i32_1 : i32
    cf.cond_br %5, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %6 = tensorium.alloca : <i32>
    %7 = tensorium.load %1 : <i32>
    %c2_i32 = arith.constant 2 : i32
    %8 = arith.muli %7, %c2_i32 : i32
    tensorium.store %8, %6 : <i32>
    %9 = tensorium.load %6 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %10 = arith.addi %9, %c1_i32 : i32
    tensorium.store %10, %6 : <i32>
    %11 = tensorium.load %0 : <i32>
    %12 = tensorium.load %6 : <i32>
    %13 = arith.addi %11, %12 : i32
    tensorium.store %13, %0 : <i32>
    %14 = tensorium.load %1 : <i32>
    %c1_i32_2 = arith.constant 1 : i32
    %15 = arith.subi %14, %c1_i32_2 : i32
    tensorium.store %15, %1 : <i32>
    cf.br ^bb1
  ^bb3:  // pred: ^bb1
    %16 = tensorium.load %0 : <i32>
    return %16 : i32
  }
  func.func @loop_address_storage(%arg0: i32) -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    %2 = tensorium.alloca : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    tensorium.store %c0_i32_0, %2 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %3 = tensorium.load %2 : <i32>
    %4 = tensorium.load %1 : <i32>
    %5 = arith.cmpi slt, %3, %4 : i32
    %6 = arith.extui %5 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %7 = arith.cmpi ne, %6, %c0_i32_1 : i32
    cf.cond_br %7, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %8 = tensorium.alloca : <i32>
    %9 = tensorium.load %2 : <i32>
    %c2_i32 = arith.constant 2 : i32
    %10 = arith.muli %9, %c2_i32 : i32
    tensorium.store %10, %8 : <i32>
    %11 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %8, %11 : <!tensorium.ptr<i32>>
    %12 = tensorium.load %11 : <!tensorium.ptr<i32>>
    %13 = tensorium.load %12 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %14 = arith.addi %13, %c1_i32 : i32
    tensorium.store %14, %12 : <i32>
    %15 = tensorium.load %0 : <i32>
    %16 = tensorium.load %8 : <i32>
    %17 = arith.addi %15, %16 : i32
    tensorium.store %17, %0 : <i32>
    cf.br ^bb3
  ^bb3:  // pred: ^bb2
    %18 = tensorium.load %2 : <i32>
    %c1_i32_2 = arith.constant 1 : i32
    %19 = arith.addi %18, %c1_i32_2 : i32
    tensorium.store %19, %2 : <i32>
    cf.br ^bb1
  ^bb4:  // pred: ^bb1
    %20 = tensorium.load %0 : <i32>
    return %20 : i32
  }
  func.func @loop_array_storage(%arg0: i32) -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    %2 = tensorium.alloca : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    tensorium.store %c0_i32_0, %2 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %3 = tensorium.load %2 : <i32>
    %4 = tensorium.load %1 : <i32>
    %5 = arith.cmpi slt, %3, %4 : i32
    %6 = arith.extui %5 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %7 = arith.cmpi ne, %6, %c0_i32_1 : i32
    cf.cond_br %7, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %8 = tensorium.alloca : <!tensorium.array<2, i32>>
    %9 = tensorium.array_decay %8 : <!tensorium.array<2, i32>> -> <i32>
    %c0_i32_2 = arith.constant 0 : i32
    %10 = arith.extsi %c0_i32_2 : i32 to i64
    %11 = tensorium.ptr_offset %9[%10] : <i32>, i64
    %12 = tensorium.load %2 : <i32>
    tensorium.store %12, %11 : <i32>
    %13 = tensorium.array_decay %8 : <!tensorium.array<2, i32>> -> <i32>
    %c1_i32 = arith.constant 1 : i32
    %14 = arith.extsi %c1_i32 : i32 to i64
    %15 = tensorium.ptr_offset %13[%14] : <i32>, i64
    %16 = tensorium.array_decay %8 : <!tensorium.array<2, i32>> -> <i32>
    %c0_i32_3 = arith.constant 0 : i32
    %17 = arith.extsi %c0_i32_3 : i32 to i64
    %18 = tensorium.ptr_offset %16[%17] : <i32>, i64
    %19 = tensorium.load %18 : <i32>
    %c3_i32 = arith.constant 3 : i32
    %20 = arith.addi %19, %c3_i32 : i32
    tensorium.store %20, %15 : <i32>
    %21 = tensorium.load %0 : <i32>
    %22 = tensorium.array_decay %8 : <!tensorium.array<2, i32>> -> <i32>
    %c0_i32_4 = arith.constant 0 : i32
    %23 = arith.extsi %c0_i32_4 : i32 to i64
    %24 = tensorium.ptr_offset %22[%23] : <i32>, i64
    %25 = tensorium.load %24 : <i32>
    %26 = tensorium.array_decay %8 : <!tensorium.array<2, i32>> -> <i32>
    %c1_i32_5 = arith.constant 1 : i32
    %27 = arith.extsi %c1_i32_5 : i32 to i64
    %28 = tensorium.ptr_offset %26[%27] : <i32>, i64
    %29 = tensorium.load %28 : <i32>
    %30 = arith.addi %25, %29 : i32
    %31 = arith.addi %21, %30 : i32
    tensorium.store %31, %0 : <i32>
    cf.br ^bb3
  ^bb3:  // pred: ^bb2
    %32 = tensorium.load %2 : <i32>
    %c1_i32_6 = arith.constant 1 : i32
    %33 = arith.addi %32, %c1_i32_6 : i32
    tensorium.store %33, %2 : <i32>
    cf.br ^bb1
  ^bb4:  // pred: ^bb1
    %34 = tensorium.load %0 : <i32>
    return %34 : i32
  }
  func.func @loop_nested_storage(%arg0: i32) -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <i32>
    tensorium.store %arg0, %1 : <i32>
    %2 = tensorium.alloca : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    tensorium.store %c0_i32_0, %2 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %3 = tensorium.load %2 : <i32>
    %4 = tensorium.load %1 : <i32>
    %5 = arith.cmpi slt, %3, %4 : i32
    %6 = arith.extui %5 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %7 = arith.cmpi ne, %6, %c0_i32_1 : i32
    cf.cond_br %7, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %8 = tensorium.alloca : <i32>
    %9 = tensorium.load %2 : <i32>
    tensorium.store %9, %8 : <i32>
    %10 = tensorium.load %8 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %11 = arith.addi %10, %c1_i32 : i32
    tensorium.store %11, %8 : <i32>
    %12 = tensorium.alloca : <i32>
    %c0_i32_2 = arith.constant 0 : i32
    tensorium.store %c0_i32_2, %12 : <i32>
    cf.br ^bb5
  ^bb3:  // pred: ^bb8
    %13 = tensorium.load %2 : <i32>
    %c1_i32_3 = arith.constant 1 : i32
    %14 = arith.addi %13, %c1_i32_3 : i32
    tensorium.store %14, %2 : <i32>
    cf.br ^bb1
  ^bb4:  // pred: ^bb1
    %15 = tensorium.load %0 : <i32>
    return %15 : i32
  ^bb5:  // 2 preds: ^bb2, ^bb7
    %16 = tensorium.load %12 : <i32>
    %c2_i32 = arith.constant 2 : i32
    %17 = arith.cmpi slt, %16, %c2_i32 : i32
    %18 = arith.extui %17 : i1 to i32
    %c0_i32_4 = arith.constant 0 : i32
    %19 = arith.cmpi ne, %18, %c0_i32_4 : i32
    cf.cond_br %19, ^bb6, ^bb8
  ^bb6:  // pred: ^bb5
    %20 = tensorium.alloca : <i32>
    %21 = tensorium.load %2 : <i32>
    %22 = tensorium.load %12 : <i32>
    %23 = arith.addi %21, %22 : i32
    tensorium.store %23, %20 : <i32>
    %24 = tensorium.load %20 : <i32>
    %c2_i32_5 = arith.constant 2 : i32
    %25 = arith.addi %24, %c2_i32_5 : i32
    tensorium.store %25, %20 : <i32>
    %26 = tensorium.load %0 : <i32>
    %27 = tensorium.load %20 : <i32>
    %28 = arith.addi %26, %27 : i32
    tensorium.store %28, %0 : <i32>
    cf.br ^bb7
  ^bb7:  // pred: ^bb6
    %29 = tensorium.load %12 : <i32>
    %c1_i32_6 = arith.constant 1 : i32
    %30 = arith.addi %29, %c1_i32_6 : i32
    tensorium.store %30, %12 : <i32>
    cf.br ^bb5
  ^bb8:  // pred: ^bb5
    %31 = tensorium.load %0 : <i32>
    %32 = tensorium.load %8 : <i32>
    %33 = arith.addi %31, %32 : i32
    tensorium.store %33, %0 : <i32>
    cf.br ^bb3
  }
  func.func private @loop_next(%arg0: !tensorium.ptr<i32>, %arg1: i32) -> i32 {
    %0 = tensorium.load %arg0 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %1 = arith.addi %0, %c1_i32 : i32
    tensorium.store %1, %arg0 : <i32>
    %2 = tensorium.load %arg0 : <i32>
    %3 = arith.addi %arg1, %2 : i32
    return %3 : i32
  }
  func.func @loop_call_initializer(%arg0: i32, %arg1: !tensorium.ptr<i32>) -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %arg1, %1 : <!tensorium.ptr<i32>>
    %2 = tensorium.alloca : <i32>
    tensorium.store %arg0, %2 : <i32>
    %3 = tensorium.alloca : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    tensorium.store %c0_i32_0, %3 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %4 = tensorium.load %3 : <i32>
    %5 = tensorium.load %2 : <i32>
    %6 = arith.cmpi slt, %4, %5 : i32
    %7 = arith.extui %6 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %8 = arith.cmpi ne, %7, %c0_i32_1 : i32
    cf.cond_br %8, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %9 = tensorium.alloca : <i32>
    %10 = tensorium.load %1 : <!tensorium.ptr<i32>>
    %11 = tensorium.load %3 : <i32>
    %c2_i32 = arith.constant 2 : i32
    %12 = arith.muli %11, %c2_i32 : i32
    %13 = call @loop_next(%10, %12) : (!tensorium.ptr<i32>, i32) -> i32
    tensorium.store %13, %9 : <i32>
    %14 = tensorium.load %9 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %15 = arith.addi %14, %c1_i32 : i32
    tensorium.store %15, %9 : <i32>
    %16 = tensorium.load %0 : <i32>
    %17 = tensorium.load %9 : <i32>
    %18 = arith.addi %16, %17 : i32
    tensorium.store %18, %0 : <i32>
    cf.br ^bb3
  ^bb3:  // pred: ^bb2
    %19 = tensorium.load %3 : <i32>
    %c1_i32_2 = arith.constant 1 : i32
    %20 = arith.addi %19, %c1_i32_2 : i32
    tensorium.store %20, %3 : <i32>
    cf.br ^bb1
  ^bb4:  // pred: ^bb1
    %21 = tensorium.load %0 : <i32>
    return %21 : i32
  }
  func.func @loop_conditional_initializer(%arg0: i32, %arg1: !tensorium.ptr<i32>) -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %arg1, %1 : <!tensorium.ptr<i32>>
    %2 = tensorium.alloca : <i32>
    tensorium.store %arg0, %2 : <i32>
    %3 = tensorium.alloca : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    tensorium.store %c0_i32_0, %3 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %4 = tensorium.load %3 : <i32>
    %5 = tensorium.load %2 : <i32>
    %6 = arith.cmpi slt, %4, %5 : i32
    %7 = arith.extui %6 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %8 = arith.cmpi ne, %7, %c0_i32_1 : i32
    cf.cond_br %8, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %9 = tensorium.load %3 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %10 = arith.cmpi eq, %9, %c1_i32 : i32
    %11 = arith.extui %10 : i1 to i32
    %c0_i32_2 = arith.constant 0 : i32
    %12 = arith.cmpi ne, %11, %c0_i32_2 : i32
    cf.cond_br %12, ^bb5, ^bb6
  ^bb3:  // 2 preds: ^bb5, ^bb8
    %13 = tensorium.load %3 : <i32>
    %c1_i32_3 = arith.constant 1 : i32
    %14 = arith.addi %13, %c1_i32_3 : i32
    tensorium.store %14, %3 : <i32>
    cf.br ^bb1
  ^bb4:  // 2 preds: ^bb1, ^bb7
    %15 = tensorium.load %0 : <i32>
    return %15 : i32
  ^bb5:  // pred: ^bb2
    cf.br ^bb3
  ^bb6:  // pred: ^bb2
    %16 = tensorium.load %3 : <i32>
    %c4_i32 = arith.constant 4 : i32
    %17 = arith.cmpi eq, %16, %c4_i32 : i32
    %18 = arith.extui %17 : i1 to i32
    %c0_i32_4 = arith.constant 0 : i32
    %19 = arith.cmpi ne, %18, %c0_i32_4 : i32
    cf.cond_br %19, ^bb7, ^bb8
  ^bb7:  // pred: ^bb6
    cf.br ^bb4
  ^bb8:  // pred: ^bb6
    %20 = tensorium.alloca : <i32>
    %21 = tensorium.load %1 : <!tensorium.ptr<i32>>
    %22 = tensorium.load %3 : <i32>
    %c2_i32 = arith.constant 2 : i32
    %23 = arith.muli %22, %c2_i32 : i32
    %24 = call @loop_next(%21, %23) : (!tensorium.ptr<i32>, i32) -> i32
    tensorium.store %24, %20 : <i32>
    %25 = tensorium.load %20 : <i32>
    %c1_i32_5 = arith.constant 1 : i32
    %26 = arith.addi %25, %c1_i32_5 : i32
    tensorium.store %26, %20 : <i32>
    %27 = tensorium.load %0 : <i32>
    %28 = tensorium.load %20 : <i32>
    %29 = arith.addi %27, %28 : i32
    tensorium.store %29, %0 : <i32>
    cf.br ^bb3
  }
  func.func @loop_load_initializer(%arg0: i32, %arg1: !tensorium.ptr<i32>) -> i32 {
    %0 = tensorium.alloca : <i32>
    %c0_i32 = arith.constant 0 : i32
    tensorium.store %c0_i32, %0 : <i32>
    %1 = tensorium.alloca : <!tensorium.ptr<i32>>
    tensorium.store %arg1, %1 : <!tensorium.ptr<i32>>
    %2 = tensorium.alloca : <i32>
    tensorium.store %arg0, %2 : <i32>
    %3 = tensorium.alloca : <i32>
    %c0_i32_0 = arith.constant 0 : i32
    tensorium.store %c0_i32_0, %3 : <i32>
    cf.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %4 = tensorium.load %3 : <i32>
    %5 = tensorium.load %2 : <i32>
    %6 = arith.cmpi slt, %4, %5 : i32
    %7 = arith.extui %6 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %8 = arith.cmpi ne, %7, %c0_i32_1 : i32
    cf.cond_br %8, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %9 = tensorium.alloca : <i32>
    %10 = tensorium.load %1 : <!tensorium.ptr<i32>>
    %11 = tensorium.load %10 : <i32>
    tensorium.store %11, %9 : <i32>
    %12 = tensorium.load %9 : <i32>
    %c1_i32 = arith.constant 1 : i32
    %13 = arith.addi %12, %c1_i32 : i32
    tensorium.store %13, %9 : <i32>
    %14 = tensorium.load %0 : <i32>
    %15 = tensorium.load %9 : <i32>
    %16 = arith.addi %14, %15 : i32
    tensorium.store %16, %0 : <i32>
    cf.br ^bb3
  ^bb3:  // pred: ^bb2
    %17 = tensorium.load %3 : <i32>
    %c1_i32_2 = arith.constant 1 : i32
    %18 = arith.addi %17, %c1_i32_2 : i32
    tensorium.store %18, %3 : <i32>
    cf.br ^bb1
  ^bb4:  // pred: ^bb1
    %19 = tensorium.load %0 : <i32>
    return %19 : i32
  }
  func.func @main() -> i32 {
    %c0_i32 = arith.constant 0 : i32
    %0 = call @loop_for_storage(%c0_i32) : (i32) -> i32
    %c0_i32_0 = arith.constant 0 : i32
    %1 = arith.cmpi ne, %0, %c0_i32_0 : i32
    %2 = arith.extui %1 : i1 to i32
    %c0_i32_1 = arith.constant 0 : i32
    %3 = arith.cmpi ne, %2, %c0_i32_1 : i32
    cf.cond_br %3, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %c1_i32 = arith.constant 1 : i32
    return %c1_i32 : i32
  ^bb2:  // pred: ^bb0
    %c4_i32 = arith.constant 4 : i32
    %4 = call @loop_for_storage(%c4_i32) : (i32) -> i32
    %c16_i32 = arith.constant 16 : i32
    %5 = arith.cmpi ne, %4, %c16_i32 : i32
    %6 = arith.extui %5 : i1 to i32
    %c0_i32_2 = arith.constant 0 : i32
    %7 = arith.cmpi ne, %6, %c0_i32_2 : i32
    cf.cond_br %7, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %c2_i32 = arith.constant 2 : i32
    return %c2_i32 : i32
  ^bb4:  // pred: ^bb2
    %c1000_i32 = arith.constant 1000 : i32
    %8 = call @loop_for_storage(%c1000_i32) : (i32) -> i32
    %c1000000_i32 = arith.constant 1000000 : i32
    %9 = arith.cmpi ne, %8, %c1000000_i32 : i32
    %10 = arith.extui %9 : i1 to i32
    %c0_i32_3 = arith.constant 0 : i32
    %11 = arith.cmpi ne, %10, %c0_i32_3 : i32
    cf.cond_br %11, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %c3_i32 = arith.constant 3 : i32
    return %c3_i32 : i32
  ^bb6:  // pred: ^bb4
    %c0_i32_4 = arith.constant 0 : i32
    %12 = call @loop_while_storage(%c0_i32_4) : (i32) -> i32
    %c0_i32_5 = arith.constant 0 : i32
    %13 = arith.cmpi ne, %12, %c0_i32_5 : i32
    %14 = arith.extui %13 : i1 to i32
    %c0_i32_6 = arith.constant 0 : i32
    %15 = arith.cmpi ne, %14, %c0_i32_6 : i32
    cf.cond_br %15, ^bb7, ^bb8
  ^bb7:  // pred: ^bb6
    %c4_i32_7 = arith.constant 4 : i32
    return %c4_i32_7 : i32
  ^bb8:  // pred: ^bb6
    %c4_i32_8 = arith.constant 4 : i32
    %16 = call @loop_while_storage(%c4_i32_8) : (i32) -> i32
    %c24_i32 = arith.constant 24 : i32
    %17 = arith.cmpi ne, %16, %c24_i32 : i32
    %18 = arith.extui %17 : i1 to i32
    %c0_i32_9 = arith.constant 0 : i32
    %19 = arith.cmpi ne, %18, %c0_i32_9 : i32
    cf.cond_br %19, ^bb9, ^bb10
  ^bb9:  // pred: ^bb8
    %c5_i32 = arith.constant 5 : i32
    return %c5_i32 : i32
  ^bb10:  // pred: ^bb8
    %c0_i32_10 = arith.constant 0 : i32
    %20 = call @loop_address_storage(%c0_i32_10) : (i32) -> i32
    %c0_i32_11 = arith.constant 0 : i32
    %21 = arith.cmpi ne, %20, %c0_i32_11 : i32
    %22 = arith.extui %21 : i1 to i32
    %c0_i32_12 = arith.constant 0 : i32
    %23 = arith.cmpi ne, %22, %c0_i32_12 : i32
    cf.cond_br %23, ^bb11, ^bb12
  ^bb11:  // pred: ^bb10
    %c6_i32 = arith.constant 6 : i32
    return %c6_i32 : i32
  ^bb12:  // pred: ^bb10
    %c4_i32_13 = arith.constant 4 : i32
    %24 = call @loop_address_storage(%c4_i32_13) : (i32) -> i32
    %c16_i32_14 = arith.constant 16 : i32
    %25 = arith.cmpi ne, %24, %c16_i32_14 : i32
    %26 = arith.extui %25 : i1 to i32
    %c0_i32_15 = arith.constant 0 : i32
    %27 = arith.cmpi ne, %26, %c0_i32_15 : i32
    cf.cond_br %27, ^bb13, ^bb14
  ^bb13:  // pred: ^bb12
    %c7_i32 = arith.constant 7 : i32
    return %c7_i32 : i32
  ^bb14:  // pred: ^bb12
    %c0_i32_16 = arith.constant 0 : i32
    %28 = call @loop_array_storage(%c0_i32_16) : (i32) -> i32
    %c0_i32_17 = arith.constant 0 : i32
    %29 = arith.cmpi ne, %28, %c0_i32_17 : i32
    %30 = arith.extui %29 : i1 to i32
    %c0_i32_18 = arith.constant 0 : i32
    %31 = arith.cmpi ne, %30, %c0_i32_18 : i32
    cf.cond_br %31, ^bb15, ^bb16
  ^bb15:  // pred: ^bb14
    %c8_i32 = arith.constant 8 : i32
    return %c8_i32 : i32
  ^bb16:  // pred: ^bb14
    %c4_i32_19 = arith.constant 4 : i32
    %32 = call @loop_array_storage(%c4_i32_19) : (i32) -> i32
    %c24_i32_20 = arith.constant 24 : i32
    %33 = arith.cmpi ne, %32, %c24_i32_20 : i32
    %34 = arith.extui %33 : i1 to i32
    %c0_i32_21 = arith.constant 0 : i32
    %35 = arith.cmpi ne, %34, %c0_i32_21 : i32
    cf.cond_br %35, ^bb17, ^bb18
  ^bb17:  // pred: ^bb16
    %c9_i32 = arith.constant 9 : i32
    return %c9_i32 : i32
  ^bb18:  // pred: ^bb16
    %c0_i32_22 = arith.constant 0 : i32
    %36 = call @loop_nested_storage(%c0_i32_22) : (i32) -> i32
    %c0_i32_23 = arith.constant 0 : i32
    %37 = arith.cmpi ne, %36, %c0_i32_23 : i32
    %38 = arith.extui %37 : i1 to i32
    %c0_i32_24 = arith.constant 0 : i32
    %39 = arith.cmpi ne, %38, %c0_i32_24 : i32
    cf.cond_br %39, ^bb19, ^bb20
  ^bb19:  // pred: ^bb18
    %c10_i32 = arith.constant 10 : i32
    return %c10_i32 : i32
  ^bb20:  // pred: ^bb18
    %c3_i32_25 = arith.constant 3 : i32
    %40 = call @loop_nested_storage(%c3_i32_25) : (i32) -> i32
    %c27_i32 = arith.constant 27 : i32
    %41 = arith.cmpi ne, %40, %c27_i32 : i32
    %42 = arith.extui %41 : i1 to i32
    %c0_i32_26 = arith.constant 0 : i32
    %43 = arith.cmpi ne, %42, %c0_i32_26 : i32
    cf.cond_br %43, ^bb21, ^bb22
  ^bb21:  // pred: ^bb20
    %c11_i32 = arith.constant 11 : i32
    return %c11_i32 : i32
  ^bb22:  // pred: ^bb20
    %44 = tensorium.alloca : <i32>
    %c0_i32_27 = arith.constant 0 : i32
    tensorium.store %c0_i32_27, %44 : <i32>
    %c0_i32_28 = arith.constant 0 : i32
    %45 = call @loop_call_initializer(%c0_i32_28, %44) : (i32, !tensorium.ptr<i32>) -> i32
    %c0_i32_29 = arith.constant 0 : i32
    %46 = arith.cmpi ne, %45, %c0_i32_29 : i32
    %47 = arith.extui %46 : i1 to i32
    %c0_i32_30 = arith.constant 0 : i32
    %48 = arith.cmpi ne, %47, %c0_i32_30 : i32
    cf.cond_br %48, ^bb23, ^bb24
  ^bb23:  // pred: ^bb22
    %c12_i32 = arith.constant 12 : i32
    return %c12_i32 : i32
  ^bb24:  // pred: ^bb22
    %49 = tensorium.load %44 : <i32>
    %c0_i32_31 = arith.constant 0 : i32
    %50 = arith.cmpi ne, %49, %c0_i32_31 : i32
    %51 = arith.extui %50 : i1 to i32
    %c0_i32_32 = arith.constant 0 : i32
    %52 = arith.cmpi ne, %51, %c0_i32_32 : i32
    cf.cond_br %52, ^bb25, ^bb26
  ^bb25:  // pred: ^bb24
    %c13_i32 = arith.constant 13 : i32
    return %c13_i32 : i32
  ^bb26:  // pred: ^bb24
    %c3_i32_33 = arith.constant 3 : i32
    %53 = call @loop_call_initializer(%c3_i32_33, %44) : (i32, !tensorium.ptr<i32>) -> i32
    %c15_i32 = arith.constant 15 : i32
    %54 = arith.cmpi ne, %53, %c15_i32 : i32
    %55 = arith.extui %54 : i1 to i32
    %c0_i32_34 = arith.constant 0 : i32
    %56 = arith.cmpi ne, %55, %c0_i32_34 : i32
    cf.cond_br %56, ^bb27, ^bb28
  ^bb27:  // pred: ^bb26
    %c14_i32 = arith.constant 14 : i32
    return %c14_i32 : i32
  ^bb28:  // pred: ^bb26
    %57 = tensorium.load %44 : <i32>
    %c3_i32_35 = arith.constant 3 : i32
    %58 = arith.cmpi ne, %57, %c3_i32_35 : i32
    %59 = arith.extui %58 : i1 to i32
    %c0_i32_36 = arith.constant 0 : i32
    %60 = arith.cmpi ne, %59, %c0_i32_36 : i32
    cf.cond_br %60, ^bb29, ^bb30
  ^bb29:  // pred: ^bb28
    %c15_i32_37 = arith.constant 15 : i32
    return %c15_i32_37 : i32
  ^bb30:  // pred: ^bb28
    %c0_i32_38 = arith.constant 0 : i32
    tensorium.store %c0_i32_38, %44 : <i32>
    %c0_i32_39 = arith.constant 0 : i32
    %61 = call @loop_conditional_initializer(%c0_i32_39, %44) : (i32, !tensorium.ptr<i32>) -> i32
    %c0_i32_40 = arith.constant 0 : i32
    %62 = arith.cmpi ne, %61, %c0_i32_40 : i32
    %63 = arith.extui %62 : i1 to i32
    %c0_i32_41 = arith.constant 0 : i32
    %64 = arith.cmpi ne, %63, %c0_i32_41 : i32
    cf.cond_br %64, ^bb31, ^bb32
  ^bb31:  // pred: ^bb30
    %c16_i32_42 = arith.constant 16 : i32
    return %c16_i32_42 : i32
  ^bb32:  // pred: ^bb30
    %65 = tensorium.load %44 : <i32>
    %c0_i32_43 = arith.constant 0 : i32
    %66 = arith.cmpi ne, %65, %c0_i32_43 : i32
    %67 = arith.extui %66 : i1 to i32
    %c0_i32_44 = arith.constant 0 : i32
    %68 = arith.cmpi ne, %67, %c0_i32_44 : i32
    cf.cond_br %68, ^bb33, ^bb34
  ^bb33:  // pred: ^bb32
    %c17_i32 = arith.constant 17 : i32
    return %c17_i32 : i32
  ^bb34:  // pred: ^bb32
    %c3_i32_45 = arith.constant 3 : i32
    %69 = call @loop_conditional_initializer(%c3_i32_45, %44) : (i32, !tensorium.ptr<i32>) -> i32
    %c9_i32_46 = arith.constant 9 : i32
    %70 = arith.cmpi ne, %69, %c9_i32_46 : i32
    %71 = arith.extui %70 : i1 to i32
    %c0_i32_47 = arith.constant 0 : i32
    %72 = arith.cmpi ne, %71, %c0_i32_47 : i32
    cf.cond_br %72, ^bb35, ^bb36
  ^bb35:  // pred: ^bb34
    %c18_i32 = arith.constant 18 : i32
    return %c18_i32 : i32
  ^bb36:  // pred: ^bb34
    %73 = tensorium.load %44 : <i32>
    %c2_i32_48 = arith.constant 2 : i32
    %74 = arith.cmpi ne, %73, %c2_i32_48 : i32
    %75 = arith.extui %74 : i1 to i32
    %c0_i32_49 = arith.constant 0 : i32
    %76 = arith.cmpi ne, %75, %c0_i32_49 : i32
    cf.cond_br %76, ^bb37, ^bb38
  ^bb37:  // pred: ^bb36
    %c19_i32 = arith.constant 19 : i32
    return %c19_i32 : i32
  ^bb38:  // pred: ^bb36
    %c0_i32_50 = arith.constant 0 : i32
    tensorium.store %c0_i32_50, %44 : <i32>
    %c7_i32_51 = arith.constant 7 : i32
    %77 = call @loop_conditional_initializer(%c7_i32_51, %44) : (i32, !tensorium.ptr<i32>) -> i32
    %c19_i32_52 = arith.constant 19 : i32
    %78 = arith.cmpi ne, %77, %c19_i32_52 : i32
    %79 = arith.extui %78 : i1 to i32
    %c0_i32_53 = arith.constant 0 : i32
    %80 = arith.cmpi ne, %79, %c0_i32_53 : i32
    cf.cond_br %80, ^bb39, ^bb40
  ^bb39:  // pred: ^bb38
    %c20_i32 = arith.constant 20 : i32
    return %c20_i32 : i32
  ^bb40:  // pred: ^bb38
    %81 = tensorium.load %44 : <i32>
    %c3_i32_54 = arith.constant 3 : i32
    %82 = arith.cmpi ne, %81, %c3_i32_54 : i32
    %83 = arith.extui %82 : i1 to i32
    %c0_i32_55 = arith.constant 0 : i32
    %84 = arith.cmpi ne, %83, %c0_i32_55 : i32
    cf.cond_br %84, ^bb41, ^bb42
  ^bb41:  // pred: ^bb40
    %c21_i32 = arith.constant 21 : i32
    return %c21_i32 : i32
  ^bb42:  // pred: ^bb40
    %c0_i32_56 = arith.constant 0 : i32
    %85 = tensorium.null : <i32>
    %86 = call @loop_load_initializer(%c0_i32_56, %85) : (i32, !tensorium.ptr<i32>) -> i32
    %c0_i32_57 = arith.constant 0 : i32
    %87 = arith.cmpi ne, %86, %c0_i32_57 : i32
    %88 = arith.extui %87 : i1 to i32
    %c0_i32_58 = arith.constant 0 : i32
    %89 = arith.cmpi ne, %88, %c0_i32_58 : i32
    cf.cond_br %89, ^bb43, ^bb44
  ^bb43:  // pred: ^bb42
    %c22_i32 = arith.constant 22 : i32
    return %c22_i32 : i32
  ^bb44:  // pred: ^bb42
    %90 = tensorium.alloca : <i32>
    %c3_i32_59 = arith.constant 3 : i32
    tensorium.store %c3_i32_59, %90 : <i32>
    %c3_i32_60 = arith.constant 3 : i32
    %91 = call @loop_load_initializer(%c3_i32_60, %90) : (i32, !tensorium.ptr<i32>) -> i32
    %c12_i32_61 = arith.constant 12 : i32
    %92 = arith.cmpi ne, %91, %c12_i32_61 : i32
    %93 = arith.extui %92 : i1 to i32
    %c0_i32_62 = arith.constant 0 : i32
    %94 = arith.cmpi ne, %93, %c0_i32_62 : i32
    cf.cond_br %94, ^bb45, ^bb46
  ^bb45:  // pred: ^bb44
    %c23_i32 = arith.constant 23 : i32
    return %c23_i32 : i32
  ^bb46:  // pred: ^bb44
    %95 = tensorium.load %90 : <i32>
    %c3_i32_63 = arith.constant 3 : i32
    %96 = arith.cmpi ne, %95, %c3_i32_63 : i32
    %97 = arith.extui %96 : i1 to i32
    %c0_i32_64 = arith.constant 0 : i32
    %98 = arith.cmpi ne, %97, %c0_i32_64 : i32
    cf.cond_br %98, ^bb47, ^bb48
  ^bb47:  // pred: ^bb46
    %c24_i32_65 = arith.constant 24 : i32
    return %c24_i32_65 : i32
  ^bb48:  // pred: ^bb46
    %c0_i32_66 = arith.constant 0 : i32
    return %c0_i32_66 : i32
  }
}

