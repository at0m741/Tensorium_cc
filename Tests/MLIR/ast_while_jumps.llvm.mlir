module {
  llvm.func @f(%arg0: i32) -> i32 {
    %0 = llvm.mlir.constant(3 : i32) : i32
    %1 = llvm.mlir.constant(1 : i32) : i32
    %2 = llvm.mlir.constant(0 : i32) : i32
    llvm.br ^bb1(%2, %arg0 : i32, i32)
  ^bb1(%3: i32, %4: i32):  // 3 preds: ^bb0, ^bb2, ^bb5
    %5 = llvm.icmp "sgt" %4, %2 : i32
    llvm.cond_br %5, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %6 = llvm.sub %4, %1 : i32
    %7 = llvm.icmp "eq" %6, %0 : i32
    llvm.cond_br %7, ^bb1(%3, %6 : i32, i32), ^bb4
  ^bb3:  // 2 preds: ^bb1, ^bb4
    llvm.return %3 : i32
  ^bb4:  // pred: ^bb2
    %8 = llvm.icmp "eq" %6, %1 : i32
    llvm.cond_br %8, ^bb3, ^bb5
  ^bb5:  // pred: ^bb4
    %9 = llvm.add %3, %1 : i32
    llvm.br ^bb1(%9, %6 : i32, i32)
  }
}

