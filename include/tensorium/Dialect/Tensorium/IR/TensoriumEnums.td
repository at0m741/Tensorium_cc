#ifndef TENSORIUM_ENUMS_TD
#define TENSORIUM_ENUMS_TD
include "mlir/IR/EnumAttr.td"

def Tensorium_PtrPredicate : I32EnumAttr<"PtrPredicate",
    "pointer comparison predicate", [
      I32EnumAttrCase<"eq", 0>, I32EnumAttrCase<"ne", 1>,
      I32EnumAttrCase<"ult", 2>, I32EnumAttrCase<"ule", 3>,
      I32EnumAttrCase<"ugt", 4>, I32EnumAttrCase<"uge", 5>
    ]> {
  let cppNamespace = "::tensorium";
}

def Tensorium_GlobalLinkage : I32EnumAttr<"GlobalLinkage",
    "global object linkage", [
      I32EnumAttrCase<"external", 0>, I32EnumAttrCase<"internal", 1>
    ]> {
  let cppNamespace = "::tensorium";
}
#endif
