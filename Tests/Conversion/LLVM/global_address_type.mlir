module {
  tensorium.global external @object : i64 {initial_value = unit}
  tensorium.global external @pointer : !tensorium.ptr<i32> {initial_value = #tensorium.global_address<@object>}
}
