module {
  func.func @bad(%p: !tensorium.ptr<!tensorium.void>) -> i64 { %v = tensorium.ptr_diff %p, %p : !tensorium.ptr<!tensorium.void> -> i64 return %v : i64 }
}
