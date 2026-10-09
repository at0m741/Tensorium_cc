; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

define i32 @f(i32 %0) {
  br label %2

2:                                                ; preds = %12, %6, %1
  %3 = phi i32 [ %13, %12 ], [ %3, %6 ], [ 0, %1 ]
  %4 = phi i32 [ %7, %12 ], [ %7, %6 ], [ %0, %1 ]
  %5 = icmp sgt i32 %4, 0
  br i1 %5, label %6, label %9

6:                                                ; preds = %2
  %7 = sub i32 %4, 1
  %8 = icmp eq i32 %7, 3
  br i1 %8, label %2, label %10

9:                                                ; preds = %10, %2
  ret i32 %3

10:                                               ; preds = %6
  %11 = icmp eq i32 %7, 1
  br i1 %11, label %9, label %12

12:                                               ; preds = %10
  %13 = add i32 %3, 1
  br label %2
}

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
