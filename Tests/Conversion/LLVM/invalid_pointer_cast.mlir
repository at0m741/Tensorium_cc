module {
  func.func @bad(%p: !tensorium.ptr<i32>) { %v = tensorium.ptr_cast %p : !tensorium.ptr<i32> -> !tensorium.ptr<f32> return }
}
