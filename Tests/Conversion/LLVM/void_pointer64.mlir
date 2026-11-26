module attributes {llvm.data_layout = "e-p:64:64"} {
  tensorium.global external @object : i32 {initial_value = 3 : i32}
  tensorium.global external @erased : !tensorium.ptr<!tensorium.void> {initial_value = #tensorium.global_address<@object>}
  func.func private @external(!tensorium.ptr<!tensorium.void>, i64) -> i64
  func.func @roundtrip(%p: !tensorium.ptr<i32>, %count: i64) -> i64 {
    %erased = tensorium.ptr_cast %p : !tensorium.ptr<i32> -> !tensorium.ptr<!tensorium.void>
    %slot = tensorium.alloca : !tensorium.ptr<!tensorium.ptr<!tensorium.void>>
    tensorium.store %erased, %slot : !tensorium.ptr<!tensorium.ptr<!tensorium.void>>
    %loaded = tensorium.load %slot : !tensorium.ptr<!tensorium.ptr<!tensorium.void>>
    %typed = tensorium.ptr_cast %loaded : !tensorium.ptr<!tensorium.void> -> !tensorium.ptr<i32>
    %unchanged = tensorium.ptr_cast %typed : !tensorium.ptr<i32> -> !tensorium.ptr<i32>
    %r = func.call @external(%loaded, %count) : (!tensorium.ptr<!tensorium.void>, i64) -> i64
    return %r : i64
  }
}
