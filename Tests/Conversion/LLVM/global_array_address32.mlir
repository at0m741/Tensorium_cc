module attributes {llvm.data_layout = "e-p:32:32"} {
  tensorium.global external @middle : !tensorium.ptr<i16> {initial_value = #tensorium.global_address<@values[0, 2]>}
  tensorium.global external @values : !tensorium.array<4, i16> {sym_visibility = "private"}
  tensorium.global external @end : !tensorium.ptr<i16> {initial_value = #tensorium.global_address<@values[0, 4]>}
  tensorium.global internal @matrix : !tensorium.array<2, !tensorium.array<3, i32>> {initial_value = unit}
  tensorium.global external @element : !tensorium.ptr<i32> {initial_value = #tensorium.global_address<@matrix[0, 1, 2]>}
  tensorium.global external @row_end : !tensorium.ptr<i32> {initial_value = #tensorium.global_address<@matrix[0, 1, 3]>}
  tensorium.global external @whole_end : !tensorium.ptr<!tensorium.array<2, !tensorium.array<3, i32>>> {initial_value = #tensorium.global_address<@matrix[1]>}
  func.func @get_middle() -> !tensorium.ptr<i16> {
    %p = tensorium.get_global @middle : !tensorium.ptr<!tensorium.ptr<i16>>
    %v = tensorium.load %p : !tensorium.ptr<!tensorium.ptr<i16>>
    return %v : !tensorium.ptr<i16>
  }
}
