module attributes {llvm.data_layout = "e-p:64:64"} {
  func.func @compare(%a: !tensorium.ptr<i32>, %b: !tensorium.ptr<i32>) -> i1 {
    %0 = tensorium.ptr_cmp eq %a, %b : !tensorium.ptr<i32>
    %1 = tensorium.ptr_cmp ne %a, %b : !tensorium.ptr<i32>
    %2 = tensorium.ptr_cmp ult %a, %b : !tensorium.ptr<i32>
    %3 = tensorium.ptr_cmp ule %a, %b : !tensorium.ptr<i32>
    %4 = tensorium.ptr_cmp ugt %a, %b : !tensorium.ptr<i32>
    %5 = tensorium.ptr_cmp uge %a, %b : !tensorium.ptr<i32>
    return %5 : i1
  }
}
