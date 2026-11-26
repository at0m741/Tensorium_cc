module attributes {llvm.data_layout = "e-p:64:64"} {
  tensorium.global external @value : i32 {initial_value = 3 : i32}
  tensorium.global internal @zero : !tensorium.array<3, i32> {initial_value = unit}
  tensorium.global external @declared : i32
  func.func @read() -> i32 {
    %p = tensorium.get_global @value : !tensorium.ptr<i32>
    %v = tensorium.load %p : !tensorium.ptr<i32>
    return %v : i32
  }
}
