#ifndef TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMDIALECT_TD
#define TENSORIUM_DIALECT_TENSORIUM_IR_TENSORIUMDIALECT_TD

include "mlir/IR/DialectBase.td"

def Tensorium_Dialect : Dialect {
  let name = "tensorium";
  let cppNamespace = "::tensorium";

  let summary = "Object and memory semantics for Tensorium";

  let description = [{
    Represents object and memory semantics before lowering to standard
    or protected memory dialects.
  }];

  let useDefaultTypePrinterParser = 1;

  let extraClassDeclaration = [{
  private:
    void registerTypes();
  }];
}

#endif
