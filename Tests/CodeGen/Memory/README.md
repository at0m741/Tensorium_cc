# Tests C de la memoire

`valid/` contient des programmes C autonomes avec `main`. Chaque programme
retourne zero si tous ses controles passent, et un code distinct pour chaque
echec. Ils couvrent l'adressage et le dereferencement, l'aliasing, les branches
et boucles, les parametres scalaires adresses, la lecture via `const` et les
conversions lors des lectures/ecritures de `short`.

`invalid/` contient les constructions actuellement refusees : stockage local
d'un pointeur, reassignment d'un parametre pointeur, adresse d'un parametre
pointeur, pointeurs imbriques, `void *`, pointees `volatile`, ecriture via
`const`. A part l'ecriture via `const`, ces restrictions viennent du backend
actuel et concernent du C valide.

Avec `CC1_ENABLE_MLIR=ON`, CTest compile chaque fichier avec `cc1 --emit-mlir`
et verifie le statut ainsi que les diagnostics attendus. Si les outils LLVM
et Clang sont disponibles, les cas valides passent aussi par :

```text
cc1 -> Tensorium -> memref -> LLVM dialect -> LLVM IR -> Clang -> execution
```

Chaque programme est execute avec `-O0` puis `-O2`. Les fichiers intermediaires
restent dans le repertoire de build, sous `Tests/CodeGen/Memory/*-runtime/`.
Le helper `Tests/CheckMLIRForRuntime.cmake` accepte un programme avec son propre
`main`, ou un fichier de pilotage separe fourni par `RUNNER`.

```sh
ctest --test-dir "$HOME/Developer/build/tensorium-llvm24" \
  -R '^mlir_memory_' --output-on-failure
```
