module {
  func.func @bad(%p: !tensorium.ptr<!tensorium.void>) { %v = tensorium.load %p : !tensorium.ptr<!tensorium.void> return }
}
