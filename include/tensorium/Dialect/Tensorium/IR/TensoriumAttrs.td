#ifndef TENSORIUM_ATTRS_TD
#define TENSORIUM_ATTRS_TD

include "mlir/IR/AttrTypeBase.td"
include "TensoriumDialect.td"

def Tensorium_GlobalAddressAttr : AttrDef<Tensorium_Dialect, "GlobalAddress"> {
  let mnemonic = "global_address";
  let summary = "Symbolic address of a global object";
  let description = [{
    Refers to the start of a global object, without loading its contents.
    The containing operation verifies symbol resolution and pointer types.
    The native linker resolves the address; no runtime initialization is needed.
  }];
  let parameters = (ins "::mlir::FlatSymbolRefAttr":$global);
  let assemblyFormat = "`<` $global `>`";
}

#endif
