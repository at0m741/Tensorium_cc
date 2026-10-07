module {
  func.func @add(%arg0: i32, %arg1: i32) -> i32 {
    %0 = arith.addi %arg0, %arg1 : i32
    return %0 : i32
  }
  func.func @main() -> i32 {
    %alloca = memref.alloca() : memref<i32>
    %c1_i32 = arith.constant 1 : i32
    memref.store %c1_i32, %alloca[] : memref<i32>
    %alloca_0 = memref.alloca() : memref<i32>
    %c4_i32 = arith.constant 4 : i32
    memref.store %c4_i32, %alloca_0[] : memref<i32>
    %0 = memref.load %alloca[] : memref<i32>
    %1 = memref.load %alloca_0[] : memref<i32>
    %2 = call @add(%0, %1) : (i32, i32) -> i32
    return %2 : i32
  }
}

