module {
  func.func @bad() { %p = tensorium.alloca : !tensorium.ptr<!tensorium.void> return }
}
