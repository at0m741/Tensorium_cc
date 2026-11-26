module attributes {llvm.data_layout = "e-p:64:64:64:16"} {
  tensorium.global external @values : !tensorium.array<65536, i8> {initial_value = unit}
  tensorium.global external @pointer : !tensorium.ptr<i8> {initial_value = #tensorium.global_address<@values[0, 32768]>}
}
