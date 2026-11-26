
#ifndef TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMOPS_TD
#define TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMOPS_TD

include "mlir/IR/OpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "TensoriumTypes.td"
include "TensoriumEnums.td"
include "mlir/IR/SymbolInterfaces.td"

class Tensorium_Op<string mnemonic, list<Trait> traits = []>
    : Op<Tensorium_Dialect, mnemonic, traits>;

include "TensoriumMemoryOps.td"
include "TensoriumGlobalOps.td"

#endif
