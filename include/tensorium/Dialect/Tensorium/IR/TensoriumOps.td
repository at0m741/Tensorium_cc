
#ifndef TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMOPS_TD
#define TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMOPS_TD

include "mlir/IR/OpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "TensoriumTypes.td"

class Tensorium_Op<string mnemonic, list<Trait> traits = []>
    : Op<Tensorium_Dialect, mnemonic, traits>;

def Tensorium_AllocaOp : Tensorium_Op<"alloca"> {
  let summary = "Allocate one local object";

  let description = [{
    Allocates one uninitialized object of the pointee type.
    Each execution creates a distinct allocation.

    Storage is released on leaving the closest enclosing operation with
    the AutomaticAllocationScope trait. The pointer must not be
    dereferenced after that scope ends.

    Size and ABI alignment are determined by the target data layout
    during lowering.
  }];

  let results = (outs
    Res<Tensorium_PtrType, "",
        [MemAlloc<AutomaticAllocationScopeResource, 0, FullEffect>]>:$result
  );

  let assemblyFormat = "attr-dict `:` type($result)";

  let hasVerifier = 1;
}

#endif
