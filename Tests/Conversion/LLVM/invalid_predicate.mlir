module {
  func.func @bad(%p: !tensorium.ptr<i32>) -> i1 {
    %r = tensorium.ptr_cmp "bogus" %p, %p : !tensorium.ptr<i32>
    return %r : i1
  }
}
