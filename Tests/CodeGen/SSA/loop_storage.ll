; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-apple-macosx13.0.0"

define i32 @loop_for_storage(i32 %0) {
  %2 = alloca i32, i64 1, align 4
  store i32 0, ptr %2, align 4
  %3 = alloca i32, i64 1, align 4
  store i32 %0, ptr %3, align 4
  %4 = alloca i32, i64 1, align 4
  store i32 0, ptr %4, align 4
  br label %5

5:                                                ; preds = %9, %1
  %6 = load i32, ptr %4, align 4
  %7 = load i32, ptr %3, align 4
  %8 = icmp slt i32 %6, %7
  br i1 %8, label %9, label %20

9:                                                ; preds = %5
  %10 = alloca i32, i64 1, align 4
  %11 = load i32, ptr %4, align 4
  %12 = mul i32 %11, 2
  store i32 %12, ptr %10, align 4
  %13 = load i32, ptr %10, align 4
  %14 = add i32 %13, 1
  store i32 %14, ptr %10, align 4
  %15 = load i32, ptr %2, align 4
  %16 = load i32, ptr %10, align 4
  %17 = add i32 %15, %16
  store i32 %17, ptr %2, align 4
  %18 = load i32, ptr %4, align 4
  %19 = add i32 %18, 1
  store i32 %19, ptr %4, align 4
  br label %5

20:                                               ; preds = %5
  %21 = load i32, ptr %2, align 4
  ret i32 %21
}

define i32 @loop_while_storage(i32 %0) {
  %2 = alloca i32, i64 1, align 4
  store i32 0, ptr %2, align 4
  %3 = alloca i32, i64 1, align 4
  store i32 %0, ptr %3, align 4
  br label %4

4:                                                ; preds = %7, %1
  %5 = load i32, ptr %3, align 4
  %6 = icmp sgt i32 %5, 0
  br i1 %6, label %7, label %18

7:                                                ; preds = %4
  %8 = alloca i32, i64 1, align 4
  %9 = load i32, ptr %3, align 4
  %10 = mul i32 %9, 2
  store i32 %10, ptr %8, align 4
  %11 = load i32, ptr %8, align 4
  %12 = add i32 %11, 1
  store i32 %12, ptr %8, align 4
  %13 = load i32, ptr %2, align 4
  %14 = load i32, ptr %8, align 4
  %15 = add i32 %13, %14
  store i32 %15, ptr %2, align 4
  %16 = load i32, ptr %3, align 4
  %17 = sub i32 %16, 1
  store i32 %17, ptr %3, align 4
  br label %4

18:                                               ; preds = %4
  %19 = load i32, ptr %2, align 4
  ret i32 %19
}

define i32 @loop_address_storage(i32 %0) {
  %2 = alloca i32, i64 1, align 4
  store i32 0, ptr %2, align 4
  %3 = alloca i32, i64 1, align 4
  store i32 %0, ptr %3, align 4
  %4 = alloca i32, i64 1, align 4
  store i32 0, ptr %4, align 4
  br label %5

5:                                                ; preds = %9, %1
  %6 = load i32, ptr %4, align 4
  %7 = load i32, ptr %3, align 4
  %8 = icmp slt i32 %6, %7
  br i1 %8, label %9, label %22

9:                                                ; preds = %5
  %10 = alloca i32, i64 1, align 4
  %11 = load i32, ptr %4, align 4
  %12 = mul i32 %11, 2
  store i32 %12, ptr %10, align 4
  %13 = alloca ptr, i64 1, align 8
  store ptr %10, ptr %13, align 8
  %14 = load ptr, ptr %13, align 8
  %15 = load i32, ptr %14, align 4
  %16 = add i32 %15, 1
  store i32 %16, ptr %14, align 4
  %17 = load i32, ptr %2, align 4
  %18 = load i32, ptr %10, align 4
  %19 = add i32 %17, %18
  store i32 %19, ptr %2, align 4
  %20 = load i32, ptr %4, align 4
  %21 = add i32 %20, 1
  store i32 %21, ptr %4, align 4
  br label %5

22:                                               ; preds = %5
  %23 = load i32, ptr %2, align 4
  ret i32 %23
}

define i32 @loop_array_storage(i32 %0) {
  %2 = alloca i32, i64 1, align 4
  store i32 0, ptr %2, align 4
  %3 = alloca i32, i64 1, align 4
  store i32 %0, ptr %3, align 4
  %4 = alloca i32, i64 1, align 4
  store i32 0, ptr %4, align 4
  br label %5

5:                                                ; preds = %9, %1
  %6 = load i32, ptr %4, align 4
  %7 = load i32, ptr %3, align 4
  %8 = icmp slt i32 %6, %7
  br i1 %8, label %9, label %31

9:                                                ; preds = %5
  %10 = alloca [2 x i32], i64 1, align 4
  %11 = getelementptr [2 x i32], ptr %10, i32 0, i32 0
  %12 = getelementptr i32, ptr %11, i64 0
  %13 = load i32, ptr %4, align 4
  store i32 %13, ptr %12, align 4
  %14 = getelementptr [2 x i32], ptr %10, i32 0, i32 0
  %15 = getelementptr i32, ptr %14, i64 1
  %16 = getelementptr [2 x i32], ptr %10, i32 0, i32 0
  %17 = getelementptr i32, ptr %16, i64 0
  %18 = load i32, ptr %17, align 4
  %19 = add i32 %18, 3
  store i32 %19, ptr %15, align 4
  %20 = load i32, ptr %2, align 4
  %21 = getelementptr [2 x i32], ptr %10, i32 0, i32 0
  %22 = getelementptr i32, ptr %21, i64 0
  %23 = load i32, ptr %22, align 4
  %24 = getelementptr [2 x i32], ptr %10, i32 0, i32 0
  %25 = getelementptr i32, ptr %24, i64 1
  %26 = load i32, ptr %25, align 4
  %27 = add i32 %23, %26
  %28 = add i32 %20, %27
  store i32 %28, ptr %2, align 4
  %29 = load i32, ptr %4, align 4
  %30 = add i32 %29, 1
  store i32 %30, ptr %4, align 4
  br label %5

31:                                               ; preds = %5
  %32 = load i32, ptr %2, align 4
  ret i32 %32
}

define i32 @loop_nested_storage(i32 %0) {
  %2 = alloca i32, i64 1, align 4
  store i32 0, ptr %2, align 4
  %3 = alloca i32, i64 1, align 4
  store i32 %0, ptr %3, align 4
  %4 = alloca i32, i64 1, align 4
  store i32 0, ptr %4, align 4
  br label %5

5:                                                ; preds = %32, %1
  %6 = load i32, ptr %4, align 4
  %7 = load i32, ptr %3, align 4
  %8 = icmp slt i32 %6, %7
  br i1 %8, label %9, label %15

9:                                                ; preds = %5
  %10 = alloca i32, i64 1, align 4
  %11 = load i32, ptr %4, align 4
  store i32 %11, ptr %10, align 4
  %12 = load i32, ptr %10, align 4
  %13 = add i32 %12, 1
  store i32 %13, ptr %10, align 4
  %14 = alloca i32, i64 1, align 4
  store i32 0, ptr %14, align 4
  br label %17

15:                                               ; preds = %5
  %16 = load i32, ptr %2, align 4
  ret i32 %16

17:                                               ; preds = %20, %9
  %18 = load i32, ptr %14, align 4
  %19 = icmp slt i32 %18, 2
  br i1 %19, label %20, label %32

20:                                               ; preds = %17
  %21 = alloca i32, i64 1, align 4
  %22 = load i32, ptr %4, align 4
  %23 = load i32, ptr %14, align 4
  %24 = add i32 %22, %23
  store i32 %24, ptr %21, align 4
  %25 = load i32, ptr %21, align 4
  %26 = add i32 %25, 2
  store i32 %26, ptr %21, align 4
  %27 = load i32, ptr %2, align 4
  %28 = load i32, ptr %21, align 4
  %29 = add i32 %27, %28
  store i32 %29, ptr %2, align 4
  %30 = load i32, ptr %14, align 4
  %31 = add i32 %30, 1
  store i32 %31, ptr %14, align 4
  br label %17

32:                                               ; preds = %17
  %33 = load i32, ptr %2, align 4
  %34 = load i32, ptr %10, align 4
  %35 = add i32 %33, %34
  store i32 %35, ptr %2, align 4
  %36 = load i32, ptr %4, align 4
  %37 = add i32 %36, 1
  store i32 %37, ptr %4, align 4
  br label %5
}

define i32 @loop_next(ptr %0, i32 %1) {
  %3 = load i32, ptr %0, align 4
  %4 = add i32 %3, 1
  store i32 %4, ptr %0, align 4
  %5 = load i32, ptr %0, align 4
  %6 = add i32 %1, %5
  ret i32 %6
}

define i32 @loop_call_initializer(i32 %0, ptr %1) {
  %3 = alloca i32, i64 1, align 4
  store i32 0, ptr %3, align 4
  %4 = alloca ptr, i64 1, align 8
  store ptr %1, ptr %4, align 8
  %5 = alloca i32, i64 1, align 4
  store i32 %0, ptr %5, align 4
  %6 = alloca i32, i64 1, align 4
  store i32 0, ptr %6, align 4
  br label %7

7:                                                ; preds = %11, %2
  %8 = load i32, ptr %6, align 4
  %9 = load i32, ptr %5, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %24

11:                                               ; preds = %7
  %12 = alloca i32, i64 1, align 4
  %13 = load ptr, ptr %4, align 8
  %14 = load i32, ptr %6, align 4
  %15 = mul i32 %14, 2
  %16 = call i32 @loop_next(ptr %13, i32 %15)
  store i32 %16, ptr %12, align 4
  %17 = load i32, ptr %12, align 4
  %18 = add i32 %17, 1
  store i32 %18, ptr %12, align 4
  %19 = load i32, ptr %3, align 4
  %20 = load i32, ptr %12, align 4
  %21 = add i32 %19, %20
  store i32 %21, ptr %3, align 4
  %22 = load i32, ptr %6, align 4
  %23 = add i32 %22, 1
  store i32 %23, ptr %6, align 4
  br label %7

24:                                               ; preds = %7
  %25 = load i32, ptr %3, align 4
  ret i32 %25
}

define i32 @loop_conditional_initializer(i32 %0, ptr %1) {
  %3 = alloca i32, i64 1, align 4
  store i32 0, ptr %3, align 4
  %4 = alloca ptr, i64 1, align 8
  store ptr %1, ptr %4, align 8
  %5 = alloca i32, i64 1, align 4
  store i32 %0, ptr %5, align 4
  %6 = alloca i32, i64 1, align 4
  store i32 0, ptr %6, align 4
  br label %7

7:                                                ; preds = %14, %2
  %8 = load i32, ptr %6, align 4
  %9 = load i32, ptr %5, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %17

11:                                               ; preds = %7
  %12 = load i32, ptr %6, align 4
  %13 = icmp eq i32 %12, 1
  br i1 %13, label %14, label %19

14:                                               ; preds = %22, %11
  %15 = load i32, ptr %6, align 4
  %16 = add i32 %15, 1
  store i32 %16, ptr %6, align 4
  br label %7

17:                                               ; preds = %19, %7
  %18 = load i32, ptr %3, align 4
  ret i32 %18

19:                                               ; preds = %11
  %20 = load i32, ptr %6, align 4
  %21 = icmp eq i32 %20, 4
  br i1 %21, label %17, label %22

22:                                               ; preds = %19
  %23 = alloca i32, i64 1, align 4
  %24 = load ptr, ptr %4, align 8
  %25 = load i32, ptr %6, align 4
  %26 = mul i32 %25, 2
  %27 = call i32 @loop_next(ptr %24, i32 %26)
  store i32 %27, ptr %23, align 4
  %28 = load i32, ptr %23, align 4
  %29 = add i32 %28, 1
  store i32 %29, ptr %23, align 4
  %30 = load i32, ptr %3, align 4
  %31 = load i32, ptr %23, align 4
  %32 = add i32 %30, %31
  store i32 %32, ptr %3, align 4
  br label %14
}

define i32 @loop_load_initializer(i32 %0, ptr %1) {
  %3 = alloca i32, i64 1, align 4
  store i32 0, ptr %3, align 4
  %4 = alloca ptr, i64 1, align 8
  store ptr %1, ptr %4, align 8
  %5 = alloca i32, i64 1, align 4
  store i32 %0, ptr %5, align 4
  %6 = alloca i32, i64 1, align 4
  store i32 0, ptr %6, align 4
  br label %7

7:                                                ; preds = %11, %2
  %8 = load i32, ptr %6, align 4
  %9 = load i32, ptr %5, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %22

11:                                               ; preds = %7
  %12 = alloca i32, i64 1, align 4
  %13 = load ptr, ptr %4, align 8
  %14 = load i32, ptr %13, align 4
  store i32 %14, ptr %12, align 4
  %15 = load i32, ptr %12, align 4
  %16 = add i32 %15, 1
  store i32 %16, ptr %12, align 4
  %17 = load i32, ptr %3, align 4
  %18 = load i32, ptr %12, align 4
  %19 = add i32 %17, %18
  store i32 %19, ptr %3, align 4
  %20 = load i32, ptr %6, align 4
  %21 = add i32 %20, 1
  store i32 %21, ptr %6, align 4
  br label %7

22:                                               ; preds = %7
  %23 = load i32, ptr %3, align 4
  ret i32 %23
}

define i32 @main() {
  %1 = call i32 @loop_for_storage(i32 0)
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %3, label %4

3:                                                ; preds = %0
  ret i32 1

4:                                                ; preds = %0
  %5 = call i32 @loop_for_storage(i32 4)
  %6 = icmp ne i32 %5, 16
  br i1 %6, label %7, label %8

7:                                                ; preds = %4
  ret i32 2

8:                                                ; preds = %4
  %9 = call i32 @loop_for_storage(i32 1000)
  %10 = icmp ne i32 %9, 1000000
  br i1 %10, label %11, label %12

11:                                               ; preds = %8
  ret i32 3

12:                                               ; preds = %8
  %13 = call i32 @loop_while_storage(i32 0)
  %14 = icmp ne i32 %13, 0
  br i1 %14, label %15, label %16

15:                                               ; preds = %12
  ret i32 4

16:                                               ; preds = %12
  %17 = call i32 @loop_while_storage(i32 4)
  %18 = icmp ne i32 %17, 24
  br i1 %18, label %19, label %20

19:                                               ; preds = %16
  ret i32 5

20:                                               ; preds = %16
  %21 = call i32 @loop_address_storage(i32 0)
  %22 = icmp ne i32 %21, 0
  br i1 %22, label %23, label %24

23:                                               ; preds = %20
  ret i32 6

24:                                               ; preds = %20
  %25 = call i32 @loop_address_storage(i32 4)
  %26 = icmp ne i32 %25, 16
  br i1 %26, label %27, label %28

27:                                               ; preds = %24
  ret i32 7

28:                                               ; preds = %24
  %29 = call i32 @loop_array_storage(i32 0)
  %30 = icmp ne i32 %29, 0
  br i1 %30, label %31, label %32

31:                                               ; preds = %28
  ret i32 8

32:                                               ; preds = %28
  %33 = call i32 @loop_array_storage(i32 4)
  %34 = icmp ne i32 %33, 24
  br i1 %34, label %35, label %36

35:                                               ; preds = %32
  ret i32 9

36:                                               ; preds = %32
  %37 = call i32 @loop_nested_storage(i32 0)
  %38 = icmp ne i32 %37, 0
  br i1 %38, label %39, label %40

39:                                               ; preds = %36
  ret i32 10

40:                                               ; preds = %36
  %41 = call i32 @loop_nested_storage(i32 3)
  %42 = icmp ne i32 %41, 27
  br i1 %42, label %43, label %44

43:                                               ; preds = %40
  ret i32 11

44:                                               ; preds = %40
  %45 = alloca i32, i64 1, align 4
  store i32 0, ptr %45, align 4
  %46 = call i32 @loop_call_initializer(i32 0, ptr %45)
  %47 = icmp ne i32 %46, 0
  br i1 %47, label %48, label %49

48:                                               ; preds = %44
  ret i32 12

49:                                               ; preds = %44
  %50 = load i32, ptr %45, align 4
  %51 = icmp ne i32 %50, 0
  br i1 %51, label %52, label %53

52:                                               ; preds = %49
  ret i32 13

53:                                               ; preds = %49
  %54 = call i32 @loop_call_initializer(i32 3, ptr %45)
  %55 = icmp ne i32 %54, 15
  br i1 %55, label %56, label %57

56:                                               ; preds = %53
  ret i32 14

57:                                               ; preds = %53
  %58 = load i32, ptr %45, align 4
  %59 = icmp ne i32 %58, 3
  br i1 %59, label %60, label %61

60:                                               ; preds = %57
  ret i32 15

61:                                               ; preds = %57
  store i32 0, ptr %45, align 4
  %62 = call i32 @loop_conditional_initializer(i32 0, ptr %45)
  %63 = icmp ne i32 %62, 0
  br i1 %63, label %64, label %65

64:                                               ; preds = %61
  ret i32 16

65:                                               ; preds = %61
  %66 = load i32, ptr %45, align 4
  %67 = icmp ne i32 %66, 0
  br i1 %67, label %68, label %69

68:                                               ; preds = %65
  ret i32 17

69:                                               ; preds = %65
  %70 = call i32 @loop_conditional_initializer(i32 3, ptr %45)
  %71 = icmp ne i32 %70, 9
  br i1 %71, label %72, label %73

72:                                               ; preds = %69
  ret i32 18

73:                                               ; preds = %69
  %74 = load i32, ptr %45, align 4
  %75 = icmp ne i32 %74, 2
  br i1 %75, label %76, label %77

76:                                               ; preds = %73
  ret i32 19

77:                                               ; preds = %73
  store i32 0, ptr %45, align 4
  %78 = call i32 @loop_conditional_initializer(i32 7, ptr %45)
  %79 = icmp ne i32 %78, 19
  br i1 %79, label %80, label %81

80:                                               ; preds = %77
  ret i32 20

81:                                               ; preds = %77
  %82 = load i32, ptr %45, align 4
  %83 = icmp ne i32 %82, 3
  br i1 %83, label %84, label %85

84:                                               ; preds = %81
  ret i32 21

85:                                               ; preds = %81
  %86 = call i32 @loop_load_initializer(i32 0, ptr null)
  %87 = icmp ne i32 %86, 0
  br i1 %87, label %88, label %89

88:                                               ; preds = %85
  ret i32 22

89:                                               ; preds = %85
  %90 = alloca i32, i64 1, align 4
  store i32 3, ptr %90, align 4
  %91 = call i32 @loop_load_initializer(i32 3, ptr %90)
  %92 = icmp ne i32 %91, 12
  br i1 %92, label %93, label %94

93:                                               ; preds = %89
  ret i32 23

94:                                               ; preds = %89
  %95 = load i32, ptr %90, align 4
  %96 = icmp ne i32 %95, 3
  br i1 %96, label %97, label %98

97:                                               ; preds = %94
  ret i32 24

98:                                               ; preds = %94
  ret i32 0
}

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
