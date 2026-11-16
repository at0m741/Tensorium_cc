module attributes {llvm.data_layout = "e-p:32:32-i64:64-n8:16:32-S128"} {
  func.func private @external(!tensorium.ptr<i32>) -> !tensorium.ptr<i32>
  func.func @advance(%p: !tensorium.ptr<!tensorium.array<3, i32>>)
      -> !tensorium.ptr<i32> {
    %one = arith.constant 1 : i32
    %row = tensorium.ptr_offset %p[%one] : !tensorium.ptr<!tensorium.array<3, i32>>, i32
    %element = tensorium.array_decay %row : !tensorium.ptr<!tensorium.array<3, i32>> -> !tensorium.ptr<i32>
    %slot = tensorium.alloca : !tensorium.ptr<!tensorium.ptr<i32>>
    tensorium.store %element, %slot : !tensorium.ptr<!tensorium.ptr<i32>>
    %loaded = tensorium.load %slot : !tensorium.ptr<!tensorium.ptr<i32>>
    %result = func.call @external(%loaded) : (!tensorium.ptr<i32>) -> !tensorium.ptr<i32>
    return %result : !tensorium.ptr<i32>
  }
  func.func @distance(%p: !tensorium.ptr<i32>, %q: !tensorium.ptr<i32>) -> i32 {
    %r = tensorium.ptr_diff %p, %q : !tensorium.ptr<i32> -> i32
    return %r : i32
  }
}
