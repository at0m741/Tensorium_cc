#ifndef TENSORIUM_MEMORY_OPS_TD
#define TENSORIUM_MEMORY_OPS_TD

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
  let hasVerifier = 1;
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
  let hasVerifier = 1;
}

def Tensorium_PtrCastOp : Tensorium_Op<"ptr_cast", [Pure]> {
  let summary = "Convert between typed object pointers and void pointers";
  let description = [{
    Preserves the address and performs no memory access. The pointer types must
    be identical, or one pointee must be void. Both use the same native address
    space and pointer representation. The caller remains responsible for object
    lifetime, alignment and valid access through a recovered typed pointer.
  }];
  let arguments = (ins Tensorium_PtrType:$source);
  let results = (outs Tensorium_PtrType:$result);
  let assemblyFormat = "$source attr-dict `:` type($source) `->` type($result)";
  let hasVerifier = 1;
}

def Tensorium_PtrOffsetOp : Tensorium_Op<"ptr_offset", [Pure,
  TypesMatchWith<"result matches the base pointer", "base", "result", "$_self">
]> {
  let summary = "Offset a pointer by a signed number of pointee objects";
  let description = [{
    Derives a pointer using the target's allocation stride of the pointee type.
    The offset is a signed integer. No memory is read and no bounds check is
    implied. C generation supplies offsets extended to the target pointer width.
  }];
  let arguments = (ins Tensorium_PtrType:$base, AnySignlessInteger:$offset);
  let results = (outs Tensorium_PtrType:$result);
  let assemblyFormat = "$base `[` $offset `]` attr-dict `:` type($base) `,` type($offset)";
  let hasVerifier = 1;
}

def Tensorium_ArrayDecayOp : Tensorium_Op<"array_decay", [Pure]> {
  let summary = "Derive a pointer to an array object's first element";
  let arguments = (ins Tensorium_PtrType:$base);
  let results = (outs Tensorium_PtrType:$result);
  let assemblyFormat = "$base attr-dict `:` type($base) `->` type($result)";
  let hasVerifier = 1;
}

def Tensorium_NullOp : Tensorium_Op<"null", [Pure]> {
  let summary = "Produce a null pointer of a specified pointee type";
  let results = (outs Tensorium_PtrType:$result);
  let assemblyFormat = "attr-dict `:` type($result)";
}

def Tensorium_PtrCmpOp : Tensorium_Op<"ptr_cmp", [Pure, SameTypeOperands]> {
  let summary = "Compare pointers using eq, ne, ult, ule, ugt or uge";
  let arguments = (ins Tensorium_PtrPredicate:$predicate, Tensorium_PtrType:$lhs,
                       Tensorium_PtrType:$rhs);
  let results = (outs I1:$result);
  let assemblyFormat = "$predicate $lhs `,` $rhs attr-dict `:` type($lhs)";
}

def Tensorium_PtrDiffOp : Tensorium_Op<"ptr_diff", [Pure, SameTypeOperands]> {
  let summary = "Compute a signed pointer difference in pointee elements";
  let description = [{
    Both pointers must belong to the same array object (or its one-past address).
    The result must be representable in the signed result integer type.
  }];
  let arguments = (ins Tensorium_PtrType:$lhs, Tensorium_PtrType:$rhs);
  let results = (outs AnySignlessInteger:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `->` type($result)";
  let hasVerifier = 1;
}

#endif
