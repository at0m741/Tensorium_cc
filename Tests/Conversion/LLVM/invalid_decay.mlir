module {
  func.func @bad(%p: !tensorium.ptr<!tensorium.array<3, i32>>)
      -> !tensorium.ptr<i64> {
    %r = tensorium.array_decay %p : !tensorium.ptr<!tensorium.array<3, i32>> -> !tensorium.ptr<i64>
    return %r : !tensorium.ptr<i64>
  }
}
