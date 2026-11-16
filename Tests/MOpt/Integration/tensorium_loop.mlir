// Tensorium memory is conservatively unknown to MOpt before conversion.
// After conversion, the invariant store has a loop-carried dependence.
module {
  func.func @update(%initial: i32) -> i32 {
    %p = tensorium.alloca : !tensorium.ptr<i32>
    tensorium.store %initial, %p : !tensorium.ptr<i32>
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c8 = arith.constant 8 : index
    scf.for %i = %c0 to %c8 step %c1 {
      %value = tensorium.load %p : !tensorium.ptr<i32>
      %one = arith.constant 1 : i32
      %next = arith.addi %value, %one : i32
      tensorium.store %next, %p : !tensorium.ptr<i32>
    }
    %result = tensorium.load %p : !tensorium.ptr<i32>
    return %result : i32
  }
}
