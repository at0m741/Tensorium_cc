module attributes {llvm.data_layout = "e-m:o-i64:64-i128:128-n32:64-S128", llvm.target_triple = "aarch64-apple-macosx13.0.0"} {
  llvm.func @loop_for_storage(%arg0: i32) -> i32 {
    %0 = llvm.mlir.constant(1 : i32) : i32
    %1 = llvm.mlir.constant(2 : i32) : i32
    %2 = llvm.mlir.constant(0 : i32) : i32
    %3 = llvm.mlir.constant(1 : i64) : i64
    %4 = llvm.alloca %3 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %4 : i32, !llvm.ptr
    %5 = llvm.mlir.constant(1 : i64) : i64
    %6 = llvm.alloca %5 x i32 : (i64) -> !llvm.ptr
    llvm.store %arg0, %6 : i32, !llvm.ptr
    %7 = llvm.mlir.constant(1 : i64) : i64
    %8 = llvm.alloca %7 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %8 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %9 = llvm.load %8 : !llvm.ptr -> i32
    %10 = llvm.load %6 : !llvm.ptr -> i32
    %11 = llvm.icmp "slt" %9, %10 : i32
    llvm.cond_br %11, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %12 = llvm.mlir.constant(1 : i64) : i64
    %13 = llvm.alloca %12 x i32 : (i64) -> !llvm.ptr
    %14 = llvm.load %8 : !llvm.ptr -> i32
    %15 = llvm.mul %14, %1 : i32
    llvm.store %15, %13 : i32, !llvm.ptr
    %16 = llvm.load %13 : !llvm.ptr -> i32
    %17 = llvm.add %16, %0 : i32
    llvm.store %17, %13 : i32, !llvm.ptr
    %18 = llvm.load %4 : !llvm.ptr -> i32
    %19 = llvm.load %13 : !llvm.ptr -> i32
    %20 = llvm.add %18, %19 : i32
    llvm.store %20, %4 : i32, !llvm.ptr
    %21 = llvm.load %8 : !llvm.ptr -> i32
    %22 = llvm.add %21, %0 : i32
    llvm.store %22, %8 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb3:  // pred: ^bb1
    %23 = llvm.load %4 : !llvm.ptr -> i32
    llvm.return %23 : i32
  }
  llvm.func @loop_while_storage(%arg0: i32) -> i32 {
    %0 = llvm.mlir.constant(1 : i32) : i32
    %1 = llvm.mlir.constant(2 : i32) : i32
    %2 = llvm.mlir.constant(0 : i32) : i32
    %3 = llvm.mlir.constant(1 : i64) : i64
    %4 = llvm.alloca %3 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %4 : i32, !llvm.ptr
    %5 = llvm.mlir.constant(1 : i64) : i64
    %6 = llvm.alloca %5 x i32 : (i64) -> !llvm.ptr
    llvm.store %arg0, %6 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %7 = llvm.load %6 : !llvm.ptr -> i32
    %8 = llvm.icmp "sgt" %7, %2 : i32
    llvm.cond_br %8, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %9 = llvm.mlir.constant(1 : i64) : i64
    %10 = llvm.alloca %9 x i32 : (i64) -> !llvm.ptr
    %11 = llvm.load %6 : !llvm.ptr -> i32
    %12 = llvm.mul %11, %1 : i32
    llvm.store %12, %10 : i32, !llvm.ptr
    %13 = llvm.load %10 : !llvm.ptr -> i32
    %14 = llvm.add %13, %0 : i32
    llvm.store %14, %10 : i32, !llvm.ptr
    %15 = llvm.load %4 : !llvm.ptr -> i32
    %16 = llvm.load %10 : !llvm.ptr -> i32
    %17 = llvm.add %15, %16 : i32
    llvm.store %17, %4 : i32, !llvm.ptr
    %18 = llvm.load %6 : !llvm.ptr -> i32
    %19 = llvm.sub %18, %0 : i32
    llvm.store %19, %6 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb3:  // pred: ^bb1
    %20 = llvm.load %4 : !llvm.ptr -> i32
    llvm.return %20 : i32
  }
  llvm.func @loop_address_storage(%arg0: i32) -> i32 {
    %0 = llvm.mlir.constant(1 : i32) : i32
    %1 = llvm.mlir.constant(2 : i32) : i32
    %2 = llvm.mlir.constant(0 : i32) : i32
    %3 = llvm.mlir.constant(1 : i64) : i64
    %4 = llvm.alloca %3 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %4 : i32, !llvm.ptr
    %5 = llvm.mlir.constant(1 : i64) : i64
    %6 = llvm.alloca %5 x i32 : (i64) -> !llvm.ptr
    llvm.store %arg0, %6 : i32, !llvm.ptr
    %7 = llvm.mlir.constant(1 : i64) : i64
    %8 = llvm.alloca %7 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %8 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %9 = llvm.load %8 : !llvm.ptr -> i32
    %10 = llvm.load %6 : !llvm.ptr -> i32
    %11 = llvm.icmp "slt" %9, %10 : i32
    llvm.cond_br %11, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %12 = llvm.mlir.constant(1 : i64) : i64
    %13 = llvm.alloca %12 x i32 : (i64) -> !llvm.ptr
    %14 = llvm.load %8 : !llvm.ptr -> i32
    %15 = llvm.mul %14, %1 : i32
    llvm.store %15, %13 : i32, !llvm.ptr
    %16 = llvm.mlir.constant(1 : i64) : i64
    %17 = llvm.alloca %16 x !llvm.ptr : (i64) -> !llvm.ptr
    llvm.store %13, %17 : !llvm.ptr, !llvm.ptr
    %18 = llvm.load %17 : !llvm.ptr -> !llvm.ptr
    %19 = llvm.load %18 : !llvm.ptr -> i32
    %20 = llvm.add %19, %0 : i32
    llvm.store %20, %18 : i32, !llvm.ptr
    %21 = llvm.load %4 : !llvm.ptr -> i32
    %22 = llvm.load %13 : !llvm.ptr -> i32
    %23 = llvm.add %21, %22 : i32
    llvm.store %23, %4 : i32, !llvm.ptr
    %24 = llvm.load %8 : !llvm.ptr -> i32
    %25 = llvm.add %24, %0 : i32
    llvm.store %25, %8 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb3:  // pred: ^bb1
    %26 = llvm.load %4 : !llvm.ptr -> i32
    llvm.return %26 : i32
  }
  llvm.func @loop_array_storage(%arg0: i32) -> i32 {
    %0 = llvm.mlir.constant(1 : i64) : i64
    %1 = llvm.mlir.constant(0 : i64) : i64
    %2 = llvm.mlir.constant(3 : i32) : i32
    %3 = llvm.mlir.constant(1 : i32) : i32
    %4 = llvm.mlir.constant(0 : i32) : i32
    %5 = llvm.mlir.constant(1 : i64) : i64
    %6 = llvm.alloca %5 x i32 : (i64) -> !llvm.ptr
    llvm.store %4, %6 : i32, !llvm.ptr
    %7 = llvm.mlir.constant(1 : i64) : i64
    %8 = llvm.alloca %7 x i32 : (i64) -> !llvm.ptr
    llvm.store %arg0, %8 : i32, !llvm.ptr
    %9 = llvm.mlir.constant(1 : i64) : i64
    %10 = llvm.alloca %9 x i32 : (i64) -> !llvm.ptr
    llvm.store %4, %10 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %11 = llvm.load %10 : !llvm.ptr -> i32
    %12 = llvm.load %8 : !llvm.ptr -> i32
    %13 = llvm.icmp "slt" %11, %12 : i32
    llvm.cond_br %13, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %14 = llvm.mlir.constant(1 : i64) : i64
    %15 = llvm.alloca %14 x !llvm.array<2 x i32> : (i64) -> !llvm.ptr
    %16 = llvm.getelementptr %15[0, 0] : (!llvm.ptr) -> !llvm.ptr, !llvm.array<2 x i32>
    %17 = llvm.getelementptr %16[%1] : (!llvm.ptr, i64) -> !llvm.ptr, i32
    %18 = llvm.load %10 : !llvm.ptr -> i32
    llvm.store %18, %17 : i32, !llvm.ptr
    %19 = llvm.getelementptr %15[0, 0] : (!llvm.ptr) -> !llvm.ptr, !llvm.array<2 x i32>
    %20 = llvm.getelementptr %19[%0] : (!llvm.ptr, i64) -> !llvm.ptr, i32
    %21 = llvm.getelementptr %15[0, 0] : (!llvm.ptr) -> !llvm.ptr, !llvm.array<2 x i32>
    %22 = llvm.getelementptr %21[%1] : (!llvm.ptr, i64) -> !llvm.ptr, i32
    %23 = llvm.load %22 : !llvm.ptr -> i32
    %24 = llvm.add %23, %2 : i32
    llvm.store %24, %20 : i32, !llvm.ptr
    %25 = llvm.load %6 : !llvm.ptr -> i32
    %26 = llvm.getelementptr %15[0, 0] : (!llvm.ptr) -> !llvm.ptr, !llvm.array<2 x i32>
    %27 = llvm.getelementptr %26[%1] : (!llvm.ptr, i64) -> !llvm.ptr, i32
    %28 = llvm.load %27 : !llvm.ptr -> i32
    %29 = llvm.getelementptr %15[0, 0] : (!llvm.ptr) -> !llvm.ptr, !llvm.array<2 x i32>
    %30 = llvm.getelementptr %29[%0] : (!llvm.ptr, i64) -> !llvm.ptr, i32
    %31 = llvm.load %30 : !llvm.ptr -> i32
    %32 = llvm.add %28, %31 : i32
    %33 = llvm.add %25, %32 : i32
    llvm.store %33, %6 : i32, !llvm.ptr
    %34 = llvm.load %10 : !llvm.ptr -> i32
    %35 = llvm.add %34, %3 : i32
    llvm.store %35, %10 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb3:  // pred: ^bb1
    %36 = llvm.load %6 : !llvm.ptr -> i32
    llvm.return %36 : i32
  }
  llvm.func @loop_nested_storage(%arg0: i32) -> i32 {
    %0 = llvm.mlir.constant(2 : i32) : i32
    %1 = llvm.mlir.constant(1 : i32) : i32
    %2 = llvm.mlir.constant(0 : i32) : i32
    %3 = llvm.mlir.constant(1 : i64) : i64
    %4 = llvm.alloca %3 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %4 : i32, !llvm.ptr
    %5 = llvm.mlir.constant(1 : i64) : i64
    %6 = llvm.alloca %5 x i32 : (i64) -> !llvm.ptr
    llvm.store %arg0, %6 : i32, !llvm.ptr
    %7 = llvm.mlir.constant(1 : i64) : i64
    %8 = llvm.alloca %7 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %8 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb6
    %9 = llvm.load %8 : !llvm.ptr -> i32
    %10 = llvm.load %6 : !llvm.ptr -> i32
    %11 = llvm.icmp "slt" %9, %10 : i32
    llvm.cond_br %11, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %12 = llvm.mlir.constant(1 : i64) : i64
    %13 = llvm.alloca %12 x i32 : (i64) -> !llvm.ptr
    %14 = llvm.load %8 : !llvm.ptr -> i32
    llvm.store %14, %13 : i32, !llvm.ptr
    %15 = llvm.load %13 : !llvm.ptr -> i32
    %16 = llvm.add %15, %1 : i32
    llvm.store %16, %13 : i32, !llvm.ptr
    %17 = llvm.mlir.constant(1 : i64) : i64
    %18 = llvm.alloca %17 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %18 : i32, !llvm.ptr
    llvm.br ^bb4
  ^bb3:  // pred: ^bb1
    %19 = llvm.load %4 : !llvm.ptr -> i32
    llvm.return %19 : i32
  ^bb4:  // 2 preds: ^bb2, ^bb5
    %20 = llvm.load %18 : !llvm.ptr -> i32
    %21 = llvm.icmp "slt" %20, %0 : i32
    llvm.cond_br %21, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %22 = llvm.mlir.constant(1 : i64) : i64
    %23 = llvm.alloca %22 x i32 : (i64) -> !llvm.ptr
    %24 = llvm.load %8 : !llvm.ptr -> i32
    %25 = llvm.load %18 : !llvm.ptr -> i32
    %26 = llvm.add %24, %25 : i32
    llvm.store %26, %23 : i32, !llvm.ptr
    %27 = llvm.load %23 : !llvm.ptr -> i32
    %28 = llvm.add %27, %0 : i32
    llvm.store %28, %23 : i32, !llvm.ptr
    %29 = llvm.load %4 : !llvm.ptr -> i32
    %30 = llvm.load %23 : !llvm.ptr -> i32
    %31 = llvm.add %29, %30 : i32
    llvm.store %31, %4 : i32, !llvm.ptr
    %32 = llvm.load %18 : !llvm.ptr -> i32
    %33 = llvm.add %32, %1 : i32
    llvm.store %33, %18 : i32, !llvm.ptr
    llvm.br ^bb4
  ^bb6:  // pred: ^bb4
    %34 = llvm.load %4 : !llvm.ptr -> i32
    %35 = llvm.load %13 : !llvm.ptr -> i32
    %36 = llvm.add %34, %35 : i32
    llvm.store %36, %4 : i32, !llvm.ptr
    %37 = llvm.load %8 : !llvm.ptr -> i32
    %38 = llvm.add %37, %1 : i32
    llvm.store %38, %8 : i32, !llvm.ptr
    llvm.br ^bb1
  }
  llvm.func @loop_next(%arg0: !llvm.ptr, %arg1: i32) -> i32 attributes {sym_visibility = "private"} {
    %0 = llvm.mlir.constant(1 : i32) : i32
    %1 = llvm.load %arg0 : !llvm.ptr -> i32
    %2 = llvm.add %1, %0 : i32
    llvm.store %2, %arg0 : i32, !llvm.ptr
    %3 = llvm.load %arg0 : !llvm.ptr -> i32
    %4 = llvm.add %arg1, %3 : i32
    llvm.return %4 : i32
  }
  llvm.func @loop_call_initializer(%arg0: i32, %arg1: !llvm.ptr) -> i32 {
    %0 = llvm.mlir.constant(1 : i32) : i32
    %1 = llvm.mlir.constant(2 : i32) : i32
    %2 = llvm.mlir.constant(0 : i32) : i32
    %3 = llvm.mlir.constant(1 : i64) : i64
    %4 = llvm.alloca %3 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %4 : i32, !llvm.ptr
    %5 = llvm.mlir.constant(1 : i64) : i64
    %6 = llvm.alloca %5 x !llvm.ptr : (i64) -> !llvm.ptr
    llvm.store %arg1, %6 : !llvm.ptr, !llvm.ptr
    %7 = llvm.mlir.constant(1 : i64) : i64
    %8 = llvm.alloca %7 x i32 : (i64) -> !llvm.ptr
    llvm.store %arg0, %8 : i32, !llvm.ptr
    %9 = llvm.mlir.constant(1 : i64) : i64
    %10 = llvm.alloca %9 x i32 : (i64) -> !llvm.ptr
    llvm.store %2, %10 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %11 = llvm.load %10 : !llvm.ptr -> i32
    %12 = llvm.load %8 : !llvm.ptr -> i32
    %13 = llvm.icmp "slt" %11, %12 : i32
    llvm.cond_br %13, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %14 = llvm.mlir.constant(1 : i64) : i64
    %15 = llvm.alloca %14 x i32 : (i64) -> !llvm.ptr
    %16 = llvm.load %6 : !llvm.ptr -> !llvm.ptr
    %17 = llvm.load %10 : !llvm.ptr -> i32
    %18 = llvm.mul %17, %1 : i32
    %19 = llvm.call @loop_next(%16, %18) : (!llvm.ptr, i32) -> i32
    llvm.store %19, %15 : i32, !llvm.ptr
    %20 = llvm.load %15 : !llvm.ptr -> i32
    %21 = llvm.add %20, %0 : i32
    llvm.store %21, %15 : i32, !llvm.ptr
    %22 = llvm.load %4 : !llvm.ptr -> i32
    %23 = llvm.load %15 : !llvm.ptr -> i32
    %24 = llvm.add %22, %23 : i32
    llvm.store %24, %4 : i32, !llvm.ptr
    %25 = llvm.load %10 : !llvm.ptr -> i32
    %26 = llvm.add %25, %0 : i32
    llvm.store %26, %10 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb3:  // pred: ^bb1
    %27 = llvm.load %4 : !llvm.ptr -> i32
    llvm.return %27 : i32
  }
  llvm.func @loop_conditional_initializer(%arg0: i32, %arg1: !llvm.ptr) -> i32 {
    %0 = llvm.mlir.constant(2 : i32) : i32
    %1 = llvm.mlir.constant(4 : i32) : i32
    %2 = llvm.mlir.constant(1 : i32) : i32
    %3 = llvm.mlir.constant(0 : i32) : i32
    %4 = llvm.mlir.constant(1 : i64) : i64
    %5 = llvm.alloca %4 x i32 : (i64) -> !llvm.ptr
    llvm.store %3, %5 : i32, !llvm.ptr
    %6 = llvm.mlir.constant(1 : i64) : i64
    %7 = llvm.alloca %6 x !llvm.ptr : (i64) -> !llvm.ptr
    llvm.store %arg1, %7 : !llvm.ptr, !llvm.ptr
    %8 = llvm.mlir.constant(1 : i64) : i64
    %9 = llvm.alloca %8 x i32 : (i64) -> !llvm.ptr
    llvm.store %arg0, %9 : i32, !llvm.ptr
    %10 = llvm.mlir.constant(1 : i64) : i64
    %11 = llvm.alloca %10 x i32 : (i64) -> !llvm.ptr
    llvm.store %3, %11 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb3
    %12 = llvm.load %11 : !llvm.ptr -> i32
    %13 = llvm.load %9 : !llvm.ptr -> i32
    %14 = llvm.icmp "slt" %12, %13 : i32
    llvm.cond_br %14, ^bb2, ^bb4
  ^bb2:  // pred: ^bb1
    %15 = llvm.load %11 : !llvm.ptr -> i32
    %16 = llvm.icmp "eq" %15, %2 : i32
    llvm.cond_br %16, ^bb3, ^bb5
  ^bb3:  // 2 preds: ^bb2, ^bb6
    %17 = llvm.load %11 : !llvm.ptr -> i32
    %18 = llvm.add %17, %2 : i32
    llvm.store %18, %11 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb4:  // 2 preds: ^bb1, ^bb5
    %19 = llvm.load %5 : !llvm.ptr -> i32
    llvm.return %19 : i32
  ^bb5:  // pred: ^bb2
    %20 = llvm.load %11 : !llvm.ptr -> i32
    %21 = llvm.icmp "eq" %20, %1 : i32
    llvm.cond_br %21, ^bb4, ^bb6
  ^bb6:  // pred: ^bb5
    %22 = llvm.mlir.constant(1 : i64) : i64
    %23 = llvm.alloca %22 x i32 : (i64) -> !llvm.ptr
    %24 = llvm.load %7 : !llvm.ptr -> !llvm.ptr
    %25 = llvm.load %11 : !llvm.ptr -> i32
    %26 = llvm.mul %25, %0 : i32
    %27 = llvm.call @loop_next(%24, %26) : (!llvm.ptr, i32) -> i32
    llvm.store %27, %23 : i32, !llvm.ptr
    %28 = llvm.load %23 : !llvm.ptr -> i32
    %29 = llvm.add %28, %2 : i32
    llvm.store %29, %23 : i32, !llvm.ptr
    %30 = llvm.load %5 : !llvm.ptr -> i32
    %31 = llvm.load %23 : !llvm.ptr -> i32
    %32 = llvm.add %30, %31 : i32
    llvm.store %32, %5 : i32, !llvm.ptr
    llvm.br ^bb3
  }
  llvm.func @loop_load_initializer(%arg0: i32, %arg1: !llvm.ptr) -> i32 {
    %0 = llvm.mlir.constant(1 : i32) : i32
    %1 = llvm.mlir.constant(0 : i32) : i32
    %2 = llvm.mlir.constant(1 : i64) : i64
    %3 = llvm.alloca %2 x i32 : (i64) -> !llvm.ptr
    llvm.store %1, %3 : i32, !llvm.ptr
    %4 = llvm.mlir.constant(1 : i64) : i64
    %5 = llvm.alloca %4 x !llvm.ptr : (i64) -> !llvm.ptr
    llvm.store %arg1, %5 : !llvm.ptr, !llvm.ptr
    %6 = llvm.mlir.constant(1 : i64) : i64
    %7 = llvm.alloca %6 x i32 : (i64) -> !llvm.ptr
    llvm.store %arg0, %7 : i32, !llvm.ptr
    %8 = llvm.mlir.constant(1 : i64) : i64
    %9 = llvm.alloca %8 x i32 : (i64) -> !llvm.ptr
    llvm.store %1, %9 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb1:  // 2 preds: ^bb0, ^bb2
    %10 = llvm.load %9 : !llvm.ptr -> i32
    %11 = llvm.load %7 : !llvm.ptr -> i32
    %12 = llvm.icmp "slt" %10, %11 : i32
    llvm.cond_br %12, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %13 = llvm.mlir.constant(1 : i64) : i64
    %14 = llvm.alloca %13 x i32 : (i64) -> !llvm.ptr
    %15 = llvm.load %5 : !llvm.ptr -> !llvm.ptr
    %16 = llvm.load %15 : !llvm.ptr -> i32
    llvm.store %16, %14 : i32, !llvm.ptr
    %17 = llvm.load %14 : !llvm.ptr -> i32
    %18 = llvm.add %17, %0 : i32
    llvm.store %18, %14 : i32, !llvm.ptr
    %19 = llvm.load %3 : !llvm.ptr -> i32
    %20 = llvm.load %14 : !llvm.ptr -> i32
    %21 = llvm.add %19, %20 : i32
    llvm.store %21, %3 : i32, !llvm.ptr
    %22 = llvm.load %9 : !llvm.ptr -> i32
    %23 = llvm.add %22, %0 : i32
    llvm.store %23, %9 : i32, !llvm.ptr
    llvm.br ^bb1
  ^bb3:  // pred: ^bb1
    %24 = llvm.load %3 : !llvm.ptr -> i32
    llvm.return %24 : i32
  }
  llvm.func @main() -> i32 {
    %0 = llvm.mlir.constant(23 : i32) : i32
    %1 = llvm.mlir.constant(22 : i32) : i32
    %2 = llvm.mlir.constant(21 : i32) : i32
    %3 = llvm.mlir.constant(20 : i32) : i32
    %4 = llvm.mlir.constant(19 : i32) : i32
    %5 = llvm.mlir.constant(18 : i32) : i32
    %6 = llvm.mlir.constant(17 : i32) : i32
    %7 = llvm.mlir.constant(14 : i32) : i32
    %8 = llvm.mlir.constant(15 : i32) : i32
    %9 = llvm.mlir.constant(13 : i32) : i32
    %10 = llvm.mlir.constant(12 : i32) : i32
    %11 = llvm.mlir.constant(11 : i32) : i32
    %12 = llvm.mlir.constant(27 : i32) : i32
    %13 = llvm.mlir.constant(10 : i32) : i32
    %14 = llvm.mlir.constant(9 : i32) : i32
    %15 = llvm.mlir.constant(8 : i32) : i32
    %16 = llvm.mlir.constant(7 : i32) : i32
    %17 = llvm.mlir.constant(6 : i32) : i32
    %18 = llvm.mlir.constant(5 : i32) : i32
    %19 = llvm.mlir.constant(24 : i32) : i32
    %20 = llvm.mlir.constant(3 : i32) : i32
    %21 = llvm.mlir.constant(1000000 : i32) : i32
    %22 = llvm.mlir.constant(1000 : i32) : i32
    %23 = llvm.mlir.constant(2 : i32) : i32
    %24 = llvm.mlir.constant(16 : i32) : i32
    %25 = llvm.mlir.constant(4 : i32) : i32
    %26 = llvm.mlir.constant(1 : i32) : i32
    %27 = llvm.mlir.constant(0 : i32) : i32
    %28 = llvm.call @loop_for_storage(%27) : (i32) -> i32
    %29 = llvm.icmp "ne" %28, %27 : i32
    llvm.cond_br %29, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    llvm.return %26 : i32
  ^bb2:  // pred: ^bb0
    %30 = llvm.call @loop_for_storage(%25) : (i32) -> i32
    %31 = llvm.icmp "ne" %30, %24 : i32
    llvm.cond_br %31, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    llvm.return %23 : i32
  ^bb4:  // pred: ^bb2
    %32 = llvm.call @loop_for_storage(%22) : (i32) -> i32
    %33 = llvm.icmp "ne" %32, %21 : i32
    llvm.cond_br %33, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    llvm.return %20 : i32
  ^bb6:  // pred: ^bb4
    %34 = llvm.call @loop_while_storage(%27) : (i32) -> i32
    %35 = llvm.icmp "ne" %34, %27 : i32
    llvm.cond_br %35, ^bb7, ^bb8
  ^bb7:  // pred: ^bb6
    llvm.return %25 : i32
  ^bb8:  // pred: ^bb6
    %36 = llvm.call @loop_while_storage(%25) : (i32) -> i32
    %37 = llvm.icmp "ne" %36, %19 : i32
    llvm.cond_br %37, ^bb9, ^bb10
  ^bb9:  // pred: ^bb8
    llvm.return %18 : i32
  ^bb10:  // pred: ^bb8
    %38 = llvm.call @loop_address_storage(%27) : (i32) -> i32
    %39 = llvm.icmp "ne" %38, %27 : i32
    llvm.cond_br %39, ^bb11, ^bb12
  ^bb11:  // pred: ^bb10
    llvm.return %17 : i32
  ^bb12:  // pred: ^bb10
    %40 = llvm.call @loop_address_storage(%25) : (i32) -> i32
    %41 = llvm.icmp "ne" %40, %24 : i32
    llvm.cond_br %41, ^bb13, ^bb14
  ^bb13:  // pred: ^bb12
    llvm.return %16 : i32
  ^bb14:  // pred: ^bb12
    %42 = llvm.call @loop_array_storage(%27) : (i32) -> i32
    %43 = llvm.icmp "ne" %42, %27 : i32
    llvm.cond_br %43, ^bb15, ^bb16
  ^bb15:  // pred: ^bb14
    llvm.return %15 : i32
  ^bb16:  // pred: ^bb14
    %44 = llvm.call @loop_array_storage(%25) : (i32) -> i32
    %45 = llvm.icmp "ne" %44, %19 : i32
    llvm.cond_br %45, ^bb17, ^bb18
  ^bb17:  // pred: ^bb16
    llvm.return %14 : i32
  ^bb18:  // pred: ^bb16
    %46 = llvm.call @loop_nested_storage(%27) : (i32) -> i32
    %47 = llvm.icmp "ne" %46, %27 : i32
    llvm.cond_br %47, ^bb19, ^bb20
  ^bb19:  // pred: ^bb18
    llvm.return %13 : i32
  ^bb20:  // pred: ^bb18
    %48 = llvm.call @loop_nested_storage(%20) : (i32) -> i32
    %49 = llvm.icmp "ne" %48, %12 : i32
    llvm.cond_br %49, ^bb21, ^bb22
  ^bb21:  // pred: ^bb20
    llvm.return %11 : i32
  ^bb22:  // pred: ^bb20
    %50 = llvm.mlir.constant(1 : i64) : i64
    %51 = llvm.alloca %50 x i32 : (i64) -> !llvm.ptr
    llvm.store %27, %51 : i32, !llvm.ptr
    %52 = llvm.call @loop_call_initializer(%27, %51) : (i32, !llvm.ptr) -> i32
    %53 = llvm.icmp "ne" %52, %27 : i32
    llvm.cond_br %53, ^bb23, ^bb24
  ^bb23:  // pred: ^bb22
    llvm.return %10 : i32
  ^bb24:  // pred: ^bb22
    %54 = llvm.load %51 : !llvm.ptr -> i32
    %55 = llvm.icmp "ne" %54, %27 : i32
    llvm.cond_br %55, ^bb25, ^bb26
  ^bb25:  // pred: ^bb24
    llvm.return %9 : i32
  ^bb26:  // pred: ^bb24
    %56 = llvm.call @loop_call_initializer(%20, %51) : (i32, !llvm.ptr) -> i32
    %57 = llvm.icmp "ne" %56, %8 : i32
    llvm.cond_br %57, ^bb27, ^bb28
  ^bb27:  // pred: ^bb26
    llvm.return %7 : i32
  ^bb28:  // pred: ^bb26
    %58 = llvm.load %51 : !llvm.ptr -> i32
    %59 = llvm.icmp "ne" %58, %20 : i32
    llvm.cond_br %59, ^bb29, ^bb30
  ^bb29:  // pred: ^bb28
    llvm.return %8 : i32
  ^bb30:  // pred: ^bb28
    llvm.store %27, %51 : i32, !llvm.ptr
    %60 = llvm.call @loop_conditional_initializer(%27, %51) : (i32, !llvm.ptr) -> i32
    %61 = llvm.icmp "ne" %60, %27 : i32
    llvm.cond_br %61, ^bb31, ^bb32
  ^bb31:  // pred: ^bb30
    llvm.return %24 : i32
  ^bb32:  // pred: ^bb30
    %62 = llvm.load %51 : !llvm.ptr -> i32
    %63 = llvm.icmp "ne" %62, %27 : i32
    llvm.cond_br %63, ^bb33, ^bb34
  ^bb33:  // pred: ^bb32
    llvm.return %6 : i32
  ^bb34:  // pred: ^bb32
    %64 = llvm.call @loop_conditional_initializer(%20, %51) : (i32, !llvm.ptr) -> i32
    %65 = llvm.icmp "ne" %64, %14 : i32
    llvm.cond_br %65, ^bb35, ^bb36
  ^bb35:  // pred: ^bb34
    llvm.return %5 : i32
  ^bb36:  // pred: ^bb34
    %66 = llvm.load %51 : !llvm.ptr -> i32
    %67 = llvm.icmp "ne" %66, %23 : i32
    llvm.cond_br %67, ^bb37, ^bb38
  ^bb37:  // pred: ^bb36
    llvm.return %4 : i32
  ^bb38:  // pred: ^bb36
    llvm.store %27, %51 : i32, !llvm.ptr
    %68 = llvm.call @loop_conditional_initializer(%16, %51) : (i32, !llvm.ptr) -> i32
    %69 = llvm.icmp "ne" %68, %4 : i32
    llvm.cond_br %69, ^bb39, ^bb40
  ^bb39:  // pred: ^bb38
    llvm.return %3 : i32
  ^bb40:  // pred: ^bb38
    %70 = llvm.load %51 : !llvm.ptr -> i32
    %71 = llvm.icmp "ne" %70, %20 : i32
    llvm.cond_br %71, ^bb41, ^bb42
  ^bb41:  // pred: ^bb40
    llvm.return %2 : i32
  ^bb42:  // pred: ^bb40
    %72 = llvm.mlir.zero : !llvm.ptr
    %73 = llvm.call @loop_load_initializer(%27, %72) : (i32, !llvm.ptr) -> i32
    %74 = llvm.icmp "ne" %73, %27 : i32
    llvm.cond_br %74, ^bb43, ^bb44
  ^bb43:  // pred: ^bb42
    llvm.return %1 : i32
  ^bb44:  // pred: ^bb42
    %75 = llvm.mlir.constant(1 : i64) : i64
    %76 = llvm.alloca %75 x i32 : (i64) -> !llvm.ptr
    llvm.store %20, %76 : i32, !llvm.ptr
    %77 = llvm.call @loop_load_initializer(%20, %76) : (i32, !llvm.ptr) -> i32
    %78 = llvm.icmp "ne" %77, %10 : i32
    llvm.cond_br %78, ^bb45, ^bb46
  ^bb45:  // pred: ^bb44
    llvm.return %0 : i32
  ^bb46:  // pred: ^bb44
    %79 = llvm.load %76 : !llvm.ptr -> i32
    %80 = llvm.icmp "ne" %79, %20 : i32
    llvm.cond_br %80, ^bb47, ^bb48
  ^bb47:  // pred: ^bb46
    llvm.return %19 : i32
  ^bb48:  // pred: ^bb46
    llvm.return %27 : i32
  }
}

