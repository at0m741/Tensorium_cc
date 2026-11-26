module {
  tensorium.global external @matrix : !tensorium.array<2, !tensorium.array<3, i32>> {initial_value = unit}
  tensorium.global external @pointer : !tensorium.ptr<i32> {initial_value = #tensorium.global_address<@matrix[0, 2, 0]>}
}
