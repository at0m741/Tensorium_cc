module {
  tensorium.global external @values : !tensorium.array<4, i8> {initial_value = unit}
  tensorium.global external @erased : !tensorium.ptr<!tensorium.void> {initial_value = #tensorium.global_address<@values[0, 5]>}
}
