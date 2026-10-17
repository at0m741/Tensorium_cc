// Raw Tensorium IR; lower with --convert-tensorium-to-memref.
module {
  func.func @add(%arg0: i32, %arg1: i32) -> i32 {
    %0 = arith.addi %arg0, %arg1 : i32
    return %0 : i32
  }
  func.func @main() -> i32 {
    %alloca = tensorium.alloca : !tensorium.ptr<i32>
    %c1_i32 = arith.constant 1 : i32
    tensorium.store %c1_i32, %alloca : !tensorium.ptr<i32>
    %alloca_0 = tensorium.alloca : !tensorium.ptr<i32>
    %c4_i32 = arith.constant 4 : i32
    tensorium.store %c4_i32, %alloca_0 : !tensorium.ptr<i32>
    %0 = tensorium.load %alloca : !tensorium.ptr<i32>
    %1 = tensorium.load %alloca_0 : !tensorium.ptr<i32>
    %2 = call @add(%0, %1) : (i32, i32) -> i32
    return %2 : i32
  }
}
