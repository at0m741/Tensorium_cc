; ModuleID = 'Tests/CodeGen/SSA/loop_storage.c'
source_filename = "Tests/CodeGen/SSA/loop_storage.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx18.0.0"

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @loop_for_storage(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  br label %6

6:                                                ; preds = %18, %1
  %7 = load i32, ptr %4, align 4
  %8 = load i32, ptr %2, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %10, label %21

10:                                               ; preds = %6
  %11 = load i32, ptr %4, align 4
  %12 = mul nsw i32 %11, 2
  store i32 %12, ptr %5, align 4
  %13 = load i32, ptr %5, align 4
  %14 = add nsw i32 %13, 1
  store i32 %14, ptr %5, align 4
  %15 = load i32, ptr %5, align 4
  %16 = load i32, ptr %3, align 4
  %17 = add nsw i32 %16, %15
  store i32 %17, ptr %3, align 4
  br label %18

18:                                               ; preds = %10
  %19 = load i32, ptr %4, align 4
  %20 = add nsw i32 %19, 1
  store i32 %20, ptr %4, align 4
  br label %6, !llvm.loop !6

21:                                               ; preds = %6
  %22 = load i32, ptr %3, align 4
  ret i32 %22
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @loop_while_storage(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  br label %5

5:                                                ; preds = %8, %1
  %6 = load i32, ptr %2, align 4
  %7 = icmp sgt i32 %6, 0
  br i1 %7, label %8, label %18

8:                                                ; preds = %5
  %9 = load i32, ptr %2, align 4
  %10 = mul nsw i32 %9, 2
  store i32 %10, ptr %4, align 4
  %11 = load i32, ptr %4, align 4
  %12 = add nsw i32 %11, 1
  store i32 %12, ptr %4, align 4
  %13 = load i32, ptr %4, align 4
  %14 = load i32, ptr %3, align 4
  %15 = add nsw i32 %14, %13
  store i32 %15, ptr %3, align 4
  %16 = load i32, ptr %2, align 4
  %17 = add nsw i32 %16, -1
  store i32 %17, ptr %2, align 4
  br label %5, !llvm.loop !8

18:                                               ; preds = %5
  %19 = load i32, ptr %3, align 4
  ret i32 %19
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @loop_address_storage(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca ptr, align 8
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  br label %7

7:                                                ; preds = %20, %1
  %8 = load i32, ptr %4, align 4
  %9 = load i32, ptr %2, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %23

11:                                               ; preds = %7
  %12 = load i32, ptr %4, align 4
  %13 = mul nsw i32 %12, 2
  store i32 %13, ptr %5, align 4
  store ptr %5, ptr %6, align 8
  %14 = load ptr, ptr %6, align 8
  %15 = load i32, ptr %14, align 4
  %16 = add nsw i32 %15, 1
  store i32 %16, ptr %14, align 4
  %17 = load i32, ptr %5, align 4
  %18 = load i32, ptr %3, align 4
  %19 = add nsw i32 %18, %17
  store i32 %19, ptr %3, align 4
  br label %20

20:                                               ; preds = %11
  %21 = load i32, ptr %4, align 4
  %22 = add nsw i32 %21, 1
  store i32 %22, ptr %4, align 4
  br label %7, !llvm.loop !9

23:                                               ; preds = %7
  %24 = load i32, ptr %3, align 4
  ret i32 %24
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @loop_array_storage(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca [2 x i32], align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  br label %6

6:                                                ; preds = %24, %1
  %7 = load i32, ptr %4, align 4
  %8 = load i32, ptr %2, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %10, label %27

10:                                               ; preds = %6
  %11 = load i32, ptr %4, align 4
  %12 = getelementptr inbounds [2 x i32], ptr %5, i64 0, i64 0
  store i32 %11, ptr %12, align 4
  %13 = getelementptr inbounds [2 x i32], ptr %5, i64 0, i64 0
  %14 = load i32, ptr %13, align 4
  %15 = add nsw i32 %14, 3
  %16 = getelementptr inbounds [2 x i32], ptr %5, i64 0, i64 1
  store i32 %15, ptr %16, align 4
  %17 = getelementptr inbounds [2 x i32], ptr %5, i64 0, i64 0
  %18 = load i32, ptr %17, align 4
  %19 = getelementptr inbounds [2 x i32], ptr %5, i64 0, i64 1
  %20 = load i32, ptr %19, align 4
  %21 = add nsw i32 %18, %20
  %22 = load i32, ptr %3, align 4
  %23 = add nsw i32 %22, %21
  store i32 %23, ptr %3, align 4
  br label %24

24:                                               ; preds = %10
  %25 = load i32, ptr %4, align 4
  %26 = add nsw i32 %25, 1
  store i32 %26, ptr %4, align 4
  br label %6, !llvm.loop !10

27:                                               ; preds = %6
  %28 = load i32, ptr %3, align 4
  ret i32 %28
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @loop_nested_storage(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  br label %8

8:                                                ; preds = %35, %1
  %9 = load i32, ptr %4, align 4
  %10 = load i32, ptr %2, align 4
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %12, label %38

12:                                               ; preds = %8
  %13 = load i32, ptr %4, align 4
  store i32 %13, ptr %5, align 4
  %14 = load i32, ptr %5, align 4
  %15 = add nsw i32 %14, 1
  store i32 %15, ptr %5, align 4
  store i32 0, ptr %6, align 4
  br label %16

16:                                               ; preds = %28, %12
  %17 = load i32, ptr %6, align 4
  %18 = icmp slt i32 %17, 2
  br i1 %18, label %19, label %31

19:                                               ; preds = %16
  %20 = load i32, ptr %4, align 4
  %21 = load i32, ptr %6, align 4
  %22 = add nsw i32 %20, %21
  store i32 %22, ptr %7, align 4
  %23 = load i32, ptr %7, align 4
  %24 = add nsw i32 %23, 2
  store i32 %24, ptr %7, align 4
  %25 = load i32, ptr %7, align 4
  %26 = load i32, ptr %3, align 4
  %27 = add nsw i32 %26, %25
  store i32 %27, ptr %3, align 4
  br label %28

28:                                               ; preds = %19
  %29 = load i32, ptr %6, align 4
  %30 = add nsw i32 %29, 1
  store i32 %30, ptr %6, align 4
  br label %16, !llvm.loop !11

31:                                               ; preds = %16
  %32 = load i32, ptr %5, align 4
  %33 = load i32, ptr %3, align 4
  %34 = add nsw i32 %33, %32
  store i32 %34, ptr %3, align 4
  br label %35

35:                                               ; preds = %31
  %36 = load i32, ptr %4, align 4
  %37 = add nsw i32 %36, 1
  store i32 %37, ptr %4, align 4
  br label %8, !llvm.loop !12

38:                                               ; preds = %8
  %39 = load i32, ptr %3, align 4
  ret i32 %39
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @loop_call_initializer(i32 noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca ptr, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store ptr %1, ptr %4, align 8
  store i32 0, ptr %5, align 4
  store i32 0, ptr %6, align 4
  br label %8

8:                                                ; preds = %22, %2
  %9 = load i32, ptr %6, align 4
  %10 = load i32, ptr %3, align 4
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %12, label %25

12:                                               ; preds = %8
  %13 = load ptr, ptr %4, align 8
  %14 = load i32, ptr %6, align 4
  %15 = mul nsw i32 %14, 2
  %16 = call i32 @loop_next(ptr noundef %13, i32 noundef %15)
  store i32 %16, ptr %7, align 4
  %17 = load i32, ptr %7, align 4
  %18 = add nsw i32 %17, 1
  store i32 %18, ptr %7, align 4
  %19 = load i32, ptr %7, align 4
  %20 = load i32, ptr %5, align 4
  %21 = add nsw i32 %20, %19
  store i32 %21, ptr %5, align 4
  br label %22

22:                                               ; preds = %12
  %23 = load i32, ptr %6, align 4
  %24 = add nsw i32 %23, 1
  store i32 %24, ptr %6, align 4
  br label %8, !llvm.loop !13

25:                                               ; preds = %8
  %26 = load i32, ptr %5, align 4
  ret i32 %26
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define internal i32 @loop_next(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 8
  %6 = load i32, ptr %5, align 4
  %7 = add nsw i32 %6, 1
  store i32 %7, ptr %5, align 4
  %8 = load i32, ptr %4, align 4
  %9 = load ptr, ptr %3, align 8
  %10 = load i32, ptr %9, align 4
  %11 = add nsw i32 %8, %10
  ret i32 %11
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @loop_conditional_initializer(i32 noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca ptr, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store ptr %1, ptr %4, align 8
  store i32 0, ptr %5, align 4
  store i32 0, ptr %6, align 4
  br label %8

8:                                                ; preds = %30, %2
  %9 = load i32, ptr %6, align 4
  %10 = load i32, ptr %3, align 4
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %12, label %33

12:                                               ; preds = %8
  %13 = load i32, ptr %6, align 4
  %14 = icmp eq i32 %13, 1
  br i1 %14, label %15, label %16

15:                                               ; preds = %12
  br label %30

16:                                               ; preds = %12
  %17 = load i32, ptr %6, align 4
  %18 = icmp eq i32 %17, 4
  br i1 %18, label %19, label %20

19:                                               ; preds = %16
  br label %33

20:                                               ; preds = %16
  %21 = load ptr, ptr %4, align 8
  %22 = load i32, ptr %6, align 4
  %23 = mul nsw i32 %22, 2
  %24 = call i32 @loop_next(ptr noundef %21, i32 noundef %23)
  store i32 %24, ptr %7, align 4
  %25 = load i32, ptr %7, align 4
  %26 = add nsw i32 %25, 1
  store i32 %26, ptr %7, align 4
  %27 = load i32, ptr %7, align 4
  %28 = load i32, ptr %5, align 4
  %29 = add nsw i32 %28, %27
  store i32 %29, ptr %5, align 4
  br label %30

30:                                               ; preds = %20, %15
  %31 = load i32, ptr %6, align 4
  %32 = add nsw i32 %31, 1
  store i32 %32, ptr %6, align 4
  br label %8, !llvm.loop !14

33:                                               ; preds = %19, %8
  %34 = load i32, ptr %5, align 4
  ret i32 %34
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @loop_load_initializer(i32 noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca ptr, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store ptr %1, ptr %4, align 8
  store i32 0, ptr %5, align 4
  store i32 0, ptr %6, align 4
  br label %8

8:                                                ; preds = %20, %2
  %9 = load i32, ptr %6, align 4
  %10 = load i32, ptr %3, align 4
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %12, label %23

12:                                               ; preds = %8
  %13 = load ptr, ptr %4, align 8
  %14 = load i32, ptr %13, align 4
  store i32 %14, ptr %7, align 4
  %15 = load i32, ptr %7, align 4
  %16 = add nsw i32 %15, 1
  store i32 %16, ptr %7, align 4
  %17 = load i32, ptr %7, align 4
  %18 = load i32, ptr %5, align 4
  %19 = add nsw i32 %18, %17
  store i32 %19, ptr %5, align 4
  br label %20

20:                                               ; preds = %12
  %21 = load i32, ptr %6, align 4
  %22 = add nsw i32 %21, 1
  store i32 %22, ptr %6, align 4
  br label %8, !llvm.loop !15

23:                                               ; preds = %8
  %24 = load i32, ptr %5, align 4
  ret i32 %24
}

; Function Attrs: noinline nounwind optnone ssp uwtable(sync)
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %4 = call i32 @loop_for_storage(i32 noundef 0)
  %5 = icmp ne i32 %4, 0
  br i1 %5, label %6, label %7

6:                                                ; preds = %0
  store i32 1, ptr %1, align 4
  br label %100

7:                                                ; preds = %0
  %8 = call i32 @loop_for_storage(i32 noundef 4)
  %9 = icmp ne i32 %8, 16
  br i1 %9, label %10, label %11

10:                                               ; preds = %7
  store i32 2, ptr %1, align 4
  br label %100

11:                                               ; preds = %7
  %12 = call i32 @loop_for_storage(i32 noundef 1000)
  %13 = icmp ne i32 %12, 1000000
  br i1 %13, label %14, label %15

14:                                               ; preds = %11
  store i32 3, ptr %1, align 4
  br label %100

15:                                               ; preds = %11
  %16 = call i32 @loop_while_storage(i32 noundef 0)
  %17 = icmp ne i32 %16, 0
  br i1 %17, label %18, label %19

18:                                               ; preds = %15
  store i32 4, ptr %1, align 4
  br label %100

19:                                               ; preds = %15
  %20 = call i32 @loop_while_storage(i32 noundef 4)
  %21 = icmp ne i32 %20, 24
  br i1 %21, label %22, label %23

22:                                               ; preds = %19
  store i32 5, ptr %1, align 4
  br label %100

23:                                               ; preds = %19
  %24 = call i32 @loop_address_storage(i32 noundef 0)
  %25 = icmp ne i32 %24, 0
  br i1 %25, label %26, label %27

26:                                               ; preds = %23
  store i32 6, ptr %1, align 4
  br label %100

27:                                               ; preds = %23
  %28 = call i32 @loop_address_storage(i32 noundef 4)
  %29 = icmp ne i32 %28, 16
  br i1 %29, label %30, label %31

30:                                               ; preds = %27
  store i32 7, ptr %1, align 4
  br label %100

31:                                               ; preds = %27
  %32 = call i32 @loop_array_storage(i32 noundef 0)
  %33 = icmp ne i32 %32, 0
  br i1 %33, label %34, label %35

34:                                               ; preds = %31
  store i32 8, ptr %1, align 4
  br label %100

35:                                               ; preds = %31
  %36 = call i32 @loop_array_storage(i32 noundef 4)
  %37 = icmp ne i32 %36, 24
  br i1 %37, label %38, label %39

38:                                               ; preds = %35
  store i32 9, ptr %1, align 4
  br label %100

39:                                               ; preds = %35
  %40 = call i32 @loop_nested_storage(i32 noundef 0)
  %41 = icmp ne i32 %40, 0
  br i1 %41, label %42, label %43

42:                                               ; preds = %39
  store i32 10, ptr %1, align 4
  br label %100

43:                                               ; preds = %39
  %44 = call i32 @loop_nested_storage(i32 noundef 3)
  %45 = icmp ne i32 %44, 27
  br i1 %45, label %46, label %47

46:                                               ; preds = %43
  store i32 11, ptr %1, align 4
  br label %100

47:                                               ; preds = %43
  store i32 0, ptr %2, align 4
  %48 = call i32 @loop_call_initializer(i32 noundef 0, ptr noundef %2)
  %49 = icmp ne i32 %48, 0
  br i1 %49, label %50, label %51

50:                                               ; preds = %47
  store i32 12, ptr %1, align 4
  br label %100

51:                                               ; preds = %47
  %52 = load i32, ptr %2, align 4
  %53 = icmp ne i32 %52, 0
  br i1 %53, label %54, label %55

54:                                               ; preds = %51
  store i32 13, ptr %1, align 4
  br label %100

55:                                               ; preds = %51
  %56 = call i32 @loop_call_initializer(i32 noundef 3, ptr noundef %2)
  %57 = icmp ne i32 %56, 15
  br i1 %57, label %58, label %59

58:                                               ; preds = %55
  store i32 14, ptr %1, align 4
  br label %100

59:                                               ; preds = %55
  %60 = load i32, ptr %2, align 4
  %61 = icmp ne i32 %60, 3
  br i1 %61, label %62, label %63

62:                                               ; preds = %59
  store i32 15, ptr %1, align 4
  br label %100

63:                                               ; preds = %59
  store i32 0, ptr %2, align 4
  %64 = call i32 @loop_conditional_initializer(i32 noundef 0, ptr noundef %2)
  %65 = icmp ne i32 %64, 0
  br i1 %65, label %66, label %67

66:                                               ; preds = %63
  store i32 16, ptr %1, align 4
  br label %100

67:                                               ; preds = %63
  %68 = load i32, ptr %2, align 4
  %69 = icmp ne i32 %68, 0
  br i1 %69, label %70, label %71

70:                                               ; preds = %67
  store i32 17, ptr %1, align 4
  br label %100

71:                                               ; preds = %67
  %72 = call i32 @loop_conditional_initializer(i32 noundef 3, ptr noundef %2)
  %73 = icmp ne i32 %72, 9
  br i1 %73, label %74, label %75

74:                                               ; preds = %71
  store i32 18, ptr %1, align 4
  br label %100

75:                                               ; preds = %71
  %76 = load i32, ptr %2, align 4
  %77 = icmp ne i32 %76, 2
  br i1 %77, label %78, label %79

78:                                               ; preds = %75
  store i32 19, ptr %1, align 4
  br label %100

79:                                               ; preds = %75
  store i32 0, ptr %2, align 4
  %80 = call i32 @loop_conditional_initializer(i32 noundef 7, ptr noundef %2)
  %81 = icmp ne i32 %80, 19
  br i1 %81, label %82, label %83

82:                                               ; preds = %79
  store i32 20, ptr %1, align 4
  br label %100

83:                                               ; preds = %79
  %84 = load i32, ptr %2, align 4
  %85 = icmp ne i32 %84, 3
  br i1 %85, label %86, label %87

86:                                               ; preds = %83
  store i32 21, ptr %1, align 4
  br label %100

87:                                               ; preds = %83
  %88 = call i32 @loop_load_initializer(i32 noundef 0, ptr noundef null)
  %89 = icmp ne i32 %88, 0
  br i1 %89, label %90, label %91

90:                                               ; preds = %87
  store i32 22, ptr %1, align 4
  br label %100

91:                                               ; preds = %87
  store i32 3, ptr %3, align 4
  %92 = call i32 @loop_load_initializer(i32 noundef 3, ptr noundef %3)
  %93 = icmp ne i32 %92, 12
  br i1 %93, label %94, label %95

94:                                               ; preds = %91
  store i32 23, ptr %1, align 4
  br label %100

95:                                               ; preds = %91
  %96 = load i32, ptr %3, align 4
  %97 = icmp ne i32 %96, 3
  br i1 %97, label %98, label %99

98:                                               ; preds = %95
  store i32 24, ptr %1, align 4
  br label %100

99:                                               ; preds = %95
  store i32 0, ptr %1, align 4
  br label %100

100:                                              ; preds = %99, %98, %94, %90, %86, %82, %78, %74, %70, %66, %62, %58, %54, %50, %46, %42, %38, %34, %30, %26, %22, %18, %14, %10, %6
  %101 = load i32, ptr %1, align 4
  ret i32 %101
}

attributes #0 = { noinline nounwind optnone ssp uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+altnzcv,+ccdp,+ccidx,+ccpp,+complxnum,+crc,+dit,+dotprod,+flagm,+fp-armv8,+fp16fml,+fptoint,+fullfp16,+jsconv,+lse,+neon,+pauth,+perfmon,+predres,+ras,+rcpc,+rdm,+sb,+sha2,+sha3,+specrestrict,+ssbs,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 27, i32 3]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 20.1.8 (https://github.com/llvm/llvm-project.git 87f0227cb60147a26a1eeb4fb06e3b505e9c7261)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
!10 = distinct !{!10, !7}
!11 = distinct !{!11, !7}
!12 = distinct !{!12, !7}
!13 = distinct !{!13, !7}
!14 = distinct !{!14, !7}
!15 = distinct !{!15, !7}
