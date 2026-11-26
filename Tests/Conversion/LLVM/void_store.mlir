module {
  func.func @bad(%p: !tensorium.ptr<!tensorium.void>, %v: !tensorium.void) { tensorium.store %v, %p : !tensorium.ptr<!tensorium.void> return }
}
