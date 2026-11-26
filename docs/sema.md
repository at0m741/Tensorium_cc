# Semantic analysis and the backend boundary

The normal frontend pipeline is `Lexer -> Preprocessor -> Parser -> Sema`.
`Sema::analyze(TranslationUnit&)` returns true only if semantic analysis finishes
without errors. The CLI runs this pipeline by default. `--dump-ast` stops after
parsing; `--dump-sema` prints the typed, converted AST after successful Sema.
Neither mode prints a partial AST after an error in its pipeline.

## Current coverage

Sema checks the subset the parser can construct: file-scope variables and
typedefs, function declarations and definitions, named parameters, nested
blocks, expression statements, returns, `if`/`else`, `while`, `for`, `break`, `continue`,
literals, identifiers, unary and binary
operators, assignments, calls, indexing, conditional expressions and `sizeof`.
Ordinary names and tags use separate namespaces. Parameters hide ordinary
globals, while references to globals and functions must have a visible declaration
at their point of use. Recursion works because a function is registered before
its body is analyzed. Compatible redeclarations share a canonical declaration;
incompatible declarations and multiple function bodies are diagnosed.

Function signatures adjust array and function parameters to pointer types.
`f(void)` has a prototype with no parameters; `f()` has an unspecified parameter
list in the selected C99 mode. Calls through prototypes check argument count and
assignment compatibility. Variadic arguments and calls without a prototype apply
default argument promotions.

Arithmetic follows integer promotions and usual arithmetic conversions, using
TargetInfo for the width of `int`, `long`, `long long` and pointers. Comparisons
and logical operators return `int`. Assignments require modifiable lvalues;
const objects, arrays, incomplete objects and temporaries cannot be assigned.
Pointer assignment checks pointee compatibility and qualifiers, including the
rejection of unsafe `T** -> const T**` conversions. Object pointers interoperate
with `void*`; function pointers do not. Pointer arithmetic and indexing require
complete object pointees. String literals have `char[N]` type, including the
terminating zero; character literals have `int` type. Numeric suffixes and
integer literal base participate in type selection within the lexer's value range.

`sizeof` checks the operand without array/function decay or a top-level lvalue
load. Its result has the target's unsigned pointer-sized integer type;
`SizeofExpr::operandType` retains the inspected type. This stage does not evaluate
`sizeof` or generate code. An invalid operand is diagnosed even though it is
unevaluated. Incomplete tentative file-scope arrays become one-element arrays
at the end of the translation unit unless a later declaration completes them.

## File-scope objects (in progress)

The parser now records file-scope initializer expressions in `VarDecl::init`.
Sema analyzes those expressions and applies initializer conversions, just as it
does for automatic locals. Compatible redeclarations share `canonicalDecl`.
`globalObjectDefinitions` tracks declarations with initializers by that canonical
identity and diagnoses a second initialized definition, with a note pointing to
the first definition. The map is reset for each analyzed translation unit.

For example, repeated compatible declarations and tentative definitions can
surround a single initialized definition:

```c
int counter;
extern int counter;
int counter = 3;
int counter;
```

Two declarations with initializers, such as `int counter = 3;` followed by
`int counter = 4;`, are diagnosed as a redefinition of the global object.

The function-prototype guard now surrounds symbol binding separately from the
global-definition check. Object linkage checks consult the canonical declaration,
including a `static` object redeclared through `extern`.

Sema checks initializer types; CodeGen invokes the shared AST integer evaluator
for the supported static initializer subset. A successful semantic-only check
does not yet establish complete C99 static-initializer legality. Tensorium global
operations and LLVM lowering are implemented for integer constant initializers,
null pointer initializers and zero-initialized objects. General address constants,
aggregate initializers and static locals remain unfinished.

The [global fixtures](../Tests/CodeGen/Globals/) include successful CodeGen,
structural checks, native execution cases and rejected initializers. See
[global object support](globals.md) for the exact scope and test commands.

## Contract for MLIR generation

After a successful analysis:

- Every reachable expression has `Expr::type`; `Expr::isLval` distinguishes an
  object address from a value. Function designators are not object lvalues.
- Every identifier has `IdentExpr::decl`, referring to the visible declaration.
  Use `decl->canonicalDecl` to associate redeclarations with one backend symbol.
- `ImplicitCastExpr` makes lvalue reads, array/function decay, arithmetic
  conversion, pointer conversion, null-pointer conversion and default argument
  promotions explicit. Its result type is `Expr::type`; its operand retains its
  original type and value category.
- Ordinary assignment retains an lvalue on the left and converts the right
  operand to the stored type. Compound assignment retains the left address and
  records `BinaryExpr::computationType`; the backend must evaluate that address
  once, load and convert to the computation type, apply the operation, convert
  back to the stored type, store, and return the stored value.
- Prefix/postfix increment and decrement also retain their lvalue operands.
  Their result is a value. The backend must preserve their differing result
  timing and evaluate the operand address once.
- Logical operators, conditionals and `sizeof` retain their evaluation semantics:
  the backend must implement short-circuit/branch evaluation and must not emit
  runtime evaluation of a `sizeof` operand.

Keep `ASTContext` and the source buffers alive while consuming this tree.
`ASTContext` owns all AST nodes, including implicit casts inserted by Sema, and
its `TypePool` owns types created by both stages. Pointers between nodes and
types are non-owning. Parser and Sema can be destroyed once their passes finish;
the checked AST remains valid. `CompilerInstance` owns the context and source
buffers for a complete invocation.

The AST and semantic analysis modules have no LLVM/MLIR dependency. The optional
backend consumes checked function signatures, parameters, expressions, calls
and returns. It emits Tensorium object storage for automatic locals, modified
parameters, nested pointers and fixed-size arrays, including address-taking.
Its AST-based `LocalStorageAnalysis` selects read-only SSA bindings before each
function is emitted. Eligible initialized locals and read-only parameters avoid
memory operations; other supported automatic objects use entry-block storage,
with initialization at the appropriate source control-flow position. This is a
CodeGen decision and does not add an LLVM/MLIR dependency to Sema.
The direct LLVM conversion implements ordinary pointer ABIs; a separate MemRef
compatibility conversion retains the original scalar subset. Global storage
generation remains unsupported. See [C memory support](memory.md).

## Boundaries of this first stage

Automatic local declarations and scalar initializers are parsed and checked,
including declaration visibility, nested scopes, typedef shadowing and initializer
conversions. File-scope initializer expressions are parsed and type-checked as
described above; their constant-expression rules remain in progress. Multiple
declarators, explicit cast syntax, additional control-flow syntax (`do`, `switch`,
`goto`) and record/enum definitions remain unsupported by the parser. Full
constant-expression checking and evaluation remain future semantic work.
Sema rejects unsupported statements rather
than silently sending them to a backend. Member access is diagnosed on incomplete
records; useful member access awaits record definitions and layout support.

Only literal array bounds are supported; VLAs and computed bounds are diagnosed.
Null-pointer constants currently cover integer literal zero and its unary `+`/`-`
forms, rather than all integer constant expressions. The lexer stores integers
in signed `long long`, so larger unsigned literals are rejected before Sema;
floating values, including long-double literals, still use double precision.
Full declaration-specifier validation, old-style function definitions, nested
prototype tag scopes and complete C99 conformance remain future work. There is
no analysis of missing returns on execution paths or unreachable code yet.
