// Deliberately use 32-bit GEP indices with 64-bit pointer representations.
module attributes {llvm.data_layout = "e-p:64:64:64:32-i64:64-n8:16:32:64-S128"} {
  func.func @allocate() -> !tensorium.ptr<!tensorium.ptr<i32>> {
    %slot = tensorium.alloca : !tensorium.ptr<!tensorium.ptr<i32>>
    %null = tensorium.null : !tensorium.ptr<i32>
    tensorium.store %null, %slot : !tensorium.ptr<!tensorium.ptr<i32>>
    return %slot : !tensorium.ptr<!tensorium.ptr<i32>>
  }
  func.func @distance(%p: !tensorium.ptr<!tensorium.array<3, i32>>,
                      %q: !tensorium.ptr<!tensorium.array<3, i32>>) -> i64 {
    %r = tensorium.ptr_diff %p, %q : !tensorium.ptr<!tensorium.array<3, i32>> -> i64
    return %r : i64
  }
}
