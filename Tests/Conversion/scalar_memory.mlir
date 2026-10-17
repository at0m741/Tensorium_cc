module {
  func.func private @identity(%p: !tensorium.ptr<i32>) -> !tensorium.ptr<i32> {
    return %p : !tensorium.ptr<i32>
  }

  func.func @tensorium_memory_i32(%value: i32, %condition: i32) -> i32 {
    %p = tensorium.alloca : !tensorium.ptr<i32>
    tensorium.store %value, %p : !tensorium.ptr<i32>
    %q = func.call @identity(%p)
      : (!tensorium.ptr<i32>) -> !tensorium.ptr<i32>
    %zero = arith.constant 0 : i32
    %choose = arith.cmpi ne, %condition, %zero : i32
    cf.cond_br %choose, ^left(%q : !tensorium.ptr<i32>),
                       ^right(%p : !tensorium.ptr<i32>)
  ^left(%a: !tensorium.ptr<i32>):
    cf.br ^exit(%a : !tensorium.ptr<i32>)
  ^right(%b: !tensorium.ptr<i32>):
    cf.br ^exit(%b : !tensorium.ptr<i32>)
  ^exit(%result: !tensorium.ptr<i32>):
    %v = tensorium.load %result : !tensorium.ptr<i32>
    return %v : i32
  }

  func.func @tensorium_memory_f64(%value: f64) -> f64 {
    %p = tensorium.alloca : !tensorium.ptr<f64>
    tensorium.store %value, %p : !tensorium.ptr<f64>
    %result = tensorium.load %p : !tensorium.ptr<f64>
    return %result : f64
  }
}
