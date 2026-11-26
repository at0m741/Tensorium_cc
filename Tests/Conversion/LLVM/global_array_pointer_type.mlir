module {
  tensorium.global external @values : !tensorium.array<4, i16> {initial_value = unit}
  tensorium.global external @pointer : !tensorium.ptr<i32> {initial_value = #tensorium.global_address<@values[0, 2]>}
}
