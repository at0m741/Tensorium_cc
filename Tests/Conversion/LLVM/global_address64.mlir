module attributes {llvm.data_layout = "e-p:64:64"} {
  tensorium.global external @pointer : !tensorium.ptr<i32> {initial_value = #tensorium.global_address<@external>}
  tensorium.global external @external : i32 {sym_visibility = "private"}
  tensorium.global internal @hidden : i32 {initial_value = 7 : i32}
  tensorium.global external @hidden_pointer : !tensorium.ptr<i32> {initial_value = #tensorium.global_address<@hidden>}
  tensorium.global external @indirect : !tensorium.ptr<!tensorium.ptr<i32>> {initial_value = #tensorium.global_address<@pointer>}
  func.func @read_pointer() -> !tensorium.ptr<i32> {
    %p = tensorium.get_global @pointer : !tensorium.ptr<!tensorium.ptr<i32>>
    %v = tensorium.load %p : !tensorium.ptr<!tensorium.ptr<i32>>
    return %v : !tensorium.ptr<i32>
  }
}
