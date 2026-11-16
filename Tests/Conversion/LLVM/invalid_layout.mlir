module attributes {llvm.data_layout = "this-is-not-a-data-layout"} {
  func.func @f() {
    return
  }
}
