#ifndef TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMTYPES_TD
#define TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMTYPES_TD

include "mlir/IR/AttrTypeBase.td"
include "TensoriumDialect.td"

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

#endif
