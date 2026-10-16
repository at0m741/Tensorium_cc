
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

def Tensorium_LoadOp : Tensorium_Op<"load", [
  TypesMatchWith<
    "result type matches the pointee type of 'address'",
    "address", "result",
    "::llvm::cast<::tensorium::PtrType>($_self).getPointeeType()"
  >
]> {
  let summary = "Read one object through a typed pointer";

  let description = [{
    Reads one complete object through a pointer.
    The result type is the pointee type.

    Requires a live, initialized object with sufficient alignment.
    This is an ordinary non-atomic, non-volatile access.
  }];

  let arguments = (ins
    Arg<Tensorium_PtrType,
        "address of the object to read",
        [MemRead]>:$address
  );

  let results = (outs AnyType:$result);

  let assemblyFormat = "$address attr-dict `:` type($address)";
}

def Tensorium_StoreOp : Tensorium_Op<"store", [
  TypesMatchWith<
    "value type matches the pointee type of 'address'",
    "address", "value",
    "::llvm::cast<::tensorium::PtrType>($_self).getPointeeType()"
  >
]> {
  let summary = "Write one object through a typed pointer";

  let description = [{
    Writes one complete object through a pointer.
    The value type must match the pointee type.

    Requires a live object with sufficient alignment.
    This is an ordinary non-atomic, non-volatile access.
  }];

  let arguments = (ins
    AnyType:$value,
    Arg<Tensorium_PtrType,
        "address of the object to write",
        [MemWrite]>:$address
  );

  let assemblyFormat =
      "$value `,` $address attr-dict `:` type($address)";
}

#endif
