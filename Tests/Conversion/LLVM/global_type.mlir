module {
  tensorium.global external @value : i32 {initial_value = 3 : i32}
  func.func @bad() -> !tensorium.ptr<i64> {
    %p = tensorium.get_global @value : !tensorium.ptr<i64>
    return %p : !tensorium.ptr<i64>
  }
}
