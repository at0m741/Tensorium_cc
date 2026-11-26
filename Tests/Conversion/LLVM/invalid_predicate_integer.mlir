module {
  func.func @bad(%p: !tensorium.ptr<i32>) -> i1 {
    %r = "tensorium.ptr_cmp"(%p, %p) {predicate = 99 : i32} : (!tensorium.ptr<i32>, !tensorium.ptr<i32>) -> i1
    return %r : i1
  }
}
