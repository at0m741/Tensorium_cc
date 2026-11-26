module {
  func.func @bad(%p: !tensorium.ptr<!tensorium.void>, %n: i64) { %v = tensorium.ptr_offset %p[%n] : !tensorium.ptr<!tensorium.void>, i64 return }
}
