module {
  tensorium.global external @object : i32 {initial_value = unit}
  tensorium.global external @value : i32 {initial_value = #tensorium.global_address<@object>}
}
