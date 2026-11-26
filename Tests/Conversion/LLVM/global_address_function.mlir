module {
  func.func private @function()
  tensorium.global external @pointer : !tensorium.ptr<i32> {initial_value = #tensorium.global_address<@function>}
}
