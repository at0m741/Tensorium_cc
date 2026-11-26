module {
  tensorium.global external @matrix : !tensorium.array<2, !tensorium.array<3, i32>> {initial_value = unit}
  tensorium.global external @erased : !tensorium.ptr<!tensorium.void> {initial_value = #tensorium.global_address<@matrix[0, 2, 0]>}
}
