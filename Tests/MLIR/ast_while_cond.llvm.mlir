module {
  llvm.func @f(%arg0: i32) -> i32 {
    %0 = llvm.mlir.constant(1 : i32) : i32
    %1 = llvm.mlir.constant(2 : i32) : i32
    %2 = llvm.mlir.constant(5 : i32) : i32
    %3 = llvm.mlir.constant(0 : i32) : i32
    llvm.br ^bb1(%3, %arg0 : i32, i32)
  ^bb1(%4: i32, %5: i32):  // 2 preds: ^bb0, ^bb6
    %6 = llvm.icmp "sgt" %5, %3 : i32
    llvm.cond_br %6, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %7 = llvm.icmp "sgt" %5, %2 : i32
    llvm.cond_br %7, ^bb4, ^bb5
  ^bb3:  // pred: ^bb1
    llvm.return %4 : i32
  ^bb4:  // pred: ^bb2
    %8 = llvm.add %4, %1 : i32
    llvm.br ^bb6(%8 : i32)
  ^bb5:  // pred: ^bb2
    %9 = llvm.add %4, %0 : i32
    llvm.br ^bb6(%9 : i32)
  ^bb6(%10: i32):  // 2 preds: ^bb4, ^bb5
    %11 = llvm.sub %5, %0 : i32
    llvm.br ^bb1(%10, %11 : i32, i32)
  }
}

