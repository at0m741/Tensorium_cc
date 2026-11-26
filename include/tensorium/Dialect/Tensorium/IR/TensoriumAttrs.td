#ifndef TENSORIUM_ATTRS_TD
#define TENSORIUM_ATTRS_TD

include "mlir/IR/AttrTypeBase.td"
include "TensoriumDialect.td"

def Tensorium_GlobalAddressAttr : AttrDef<Tensorium_Dialect, "GlobalAddress"> {
  let mnemonic = "global_address";
  let summary = "Symbolic address of a global object";
  let description = [{
    Refers to a global object or an element, without loading its contents.
    Optional indices form a typed GEP path: object offset followed by array
    indices. Each step stays within its array, including a final one-past
    position. A one-past position cannot be traversed into an inner array.
    The containing operation verifies symbol resolution and pointer types.
    The native linker resolves the address; no runtime initialization is needed.
  }];
  let parameters = (ins "::mlir::FlatSymbolRefAttr":$global,
    OptionalArrayRefParameter<"int64_t">:$indices);
  let assemblyFormat = "`<` $global (`[` $indices^ `]`)? `>`";
  let builders = [AttrBuilder<(ins "::mlir::FlatSymbolRefAttr":$global), [{
    return $_get($_ctxt, global, ::llvm::ArrayRef<int64_t>{});
  }]>];
  let extraClassDeclaration = [{
    ::mlir::FailureOr<::mlir::Type> getPointeeType(::mlir::Type root) const;
  }];
}

#endif
