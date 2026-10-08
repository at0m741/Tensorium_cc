; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

define i32 @f(i32 %0) {
  br label %2

2:                                                ; preds = %13, %1
  %3 = phi i32 [ %14, %13 ], [ 0, %1 ]
  %4 = phi i32 [ %15, %13 ], [ %0, %1 ]
  %5 = icmp sgt i32 %4, 0
  br i1 %5, label %6, label %8

6:                                                ; preds = %2
  %7 = icmp sgt i32 %4, 5
  br i1 %7, label %9, label %11

8:                                                ; preds = %2
  ret i32 %3

9:                                                ; preds = %6
  %10 = add i32 %3, 2
  br label %13

11:                                               ; preds = %6
  %12 = add i32 %3, 1
  br label %13

13:                                               ; preds = %9, %11
  %14 = phi i32 [ %12, %11 ], [ %10, %9 ]
  %15 = sub i32 %4, 1
  br label %2
}

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
