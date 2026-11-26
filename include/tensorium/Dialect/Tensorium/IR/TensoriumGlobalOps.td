#ifndef TENSORIUM_GLOBAL_OPS_TD
#define TENSORIUM_GLOBAL_OPS_TD

def Tensorium_GlobalOp : Tensorium_Op<"global", [Symbol]> {
  let summary = "Declare or define a global object";
  let description = [{
    An absent initial_value denotes a declaration. A unit initial_value denotes
    a zero-initialized definition. An integer attribute defines an integer
    object with that value. Internal linkage requires a definition.
    Writing through a pointer to a constant definition is undefined.
    Linkage describes native linkage, independently of MLIR symbol visibility.
    Declarations require non-public MLIR symbol visibility, normally private,
    even when their native linkage is external.
  }];
  let arguments = (ins SymbolNameAttr:$sym_name, TypeAttr:$object_type,
    Tensorium_GlobalLinkage:$linkage, OptionalAttr<AnyAttr>:$initial_value,
    UnitAttr:$constant);
  let assemblyFormat = "$linkage $sym_name `:` $object_type attr-dict";
  let extraClassDeclaration = [{
    ::mlir::StringAttr getNameAttr() { return getSymNameAttr(); }
    ::mlir::SymbolTable::Visibility getVisibility() {
      // SymbolTable dispatches through this interface: read storage directly.
      auto attr = (*this)->getAttrOfType<::mlir::StringAttr>(
          "sym_visibility");
      if (!attr || attr.getValue() == "public")
        return ::mlir::SymbolTable::Visibility::Public;
      return attr.getValue() == "private"
          ? ::mlir::SymbolTable::Visibility::Private
          : ::mlir::SymbolTable::Visibility::Nested;
    }
    void setVisibility(::mlir::SymbolTable::Visibility visibility) {
      auto name = "sym_visibility";
      if (visibility == ::mlir::SymbolTable::Visibility::Public) {
        (*this)->removeAttr(name);
        return;
      }
      (*this)->setAttr(name, ::mlir::StringAttr::get(getContext(),
          visibility == ::mlir::SymbolTable::Visibility::Private
              ? "private" : "nested"));
    }
    bool isDeclaration() { return !getInitialValue(); }
  }];
  let hasVerifier = 1;
}

def Tensorium_GetGlobalOp : Tensorium_Op<"get_global", [Pure,
    DeclareOpInterfaceMethods<SymbolUserOpInterface>]> {
  let summary = "Obtain a typed pointer to a global object";
  let arguments = (ins FlatSymbolRefAttr:$name);
  let results = (outs Tensorium_PtrType:$result);
  let assemblyFormat = "$name `:` type($result) attr-dict";
}
#endif
