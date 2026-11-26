module {
  func.func @bad() -> !tensorium.ptr<i32> {
    %p = tensorium.get_global @missing : !tensorium.ptr<i32>
    return %p : !tensorium.ptr<i32>
  }
}
