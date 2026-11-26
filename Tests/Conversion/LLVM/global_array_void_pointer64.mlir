module attributes {llvm.data_layout = "e-p:64:64"} {
  tensorium.global external @values : !tensorium.array<4, i8> {initial_value = unit}
  tensorium.global external @erased : !tensorium.ptr<!tensorium.void> {initial_value = #tensorium.global_address<@values[0, 1]>}
  func.func @get_erased() -> !tensorium.ptr<!tensorium.void> {
    %p = tensorium.get_global @erased : !tensorium.ptr<!tensorium.ptr<!tensorium.void>>
    %value = tensorium.load %p : !tensorium.ptr<!tensorium.ptr<!tensorium.void>>
    return %value : !tensorium.ptr<!tensorium.void>
  }
}
