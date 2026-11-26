#ifndef TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMTYPES_TD
#define TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMTYPES_TD

include "mlir/IR/AttrTypeBase.td"
include "TensoriumDialect.td"

def Tensorium_VoidType : TypeDef<Tensorium_Dialect, "Void"> {
  let mnemonic = "void";
  let summary = "Incomplete pointee marker for C void pointers";
  let description = [{
    Used inside ptr<void> to represent an object address without an element
    type. This marker has no object size. It cannot be allocated, loaded,
    stored, used as an array element, or used for pointer arithmetic.
  }];
}

def Tensorium_PtrType : TypeDef<Tensorium_Dialect, "Ptr"> {
  let mnemonic = "ptr";

  let summary = "Typed pointer to a memory object";

  let description = [{
    Represents a pointer to an object of the specified pointee type.
    Its concrete representation is selected during lowering.

    Example: !tensorium.ptr<i32>
  }];

  let parameters = (ins "::mlir::Type":$pointeeType);

  let assemblyFormat = "`<` $pointeeType `>`";

  let genVerifyDecl = 1;
}

def Tensorium_ArrayType : TypeDef<Tensorium_Dialect, "Array"> {
  let mnemonic = "array";
  let summary = "Fixed-size contiguous array object";
  let description = [{
    An object containing size consecutive elements. This describes memory
    storage, not an SSA tensor value. Arrays can contain other arrays or typed
    pointers. Size, element stride and ABI alignment follow the target layout.
  }];
  let parameters = (ins "uint64_t":$size, "::mlir::Type":$elementType);
  let assemblyFormat = "`<` $size `,` $elementType `>`";
  let genVerifyDecl = 1;
}

#endif
