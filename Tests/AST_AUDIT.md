# Audit du parser et des dumps AST

29 fichiers C examines, dont 15 nouveaux cas `ast_*.c` et `invalid_*.c`.
Les sorties completes sont dans `build/ast-audit/report.md`, avec un fichier
`.ast`, `.stderr` et `.status` pour chaque entree, y compris celles qui echouent.

La compilation passe. CTest : **25 entrees passent sur 27**. Les deux entrees
en echec sont `cli_ast_invalid_parameter_list` et `parser_tests`. Dans cette
derniere, **17 cas passent sur 22** ; les 5 nouveaux echecs reproduisent les
defauts de structure et d'affichage decrits ci-dessous.

## Resultats des executions

| Resultat de `--dump-ast` | Fichiers |
|---|---:|
| Retour 0 et dump identique au texte attendu | 10 |
| Retour 0 mais presence de `UnsupportedNode` | 8 |
| Retour 0 mais declarations de tags perdues (`mixed_structs.c`) | 1 |
| Retour 1 pour une erreur syntaxique/lexicale volontaire | 5 |
| Retour 1 sur une construction actuellement non prise en charge | 4 |
| Segmentation fault | 1 |

Les 10 dumps de reference verifient notamment les priorites, le groupement,
l'associativite a droite des affectations, les blocs imbriques, les instructions
vides, `return;`, les prototypes et les declarations simples. La conformite du
texte du dump ne garantit pas la presence de toutes les donnees dans l'AST :
`ast_assignments.c`, par exemple, affiche l'arbre attendu mais perd ses parametres.

## Defauts confirmes

### 1. Crash sur une liste de parametres malformee

- Reproduction : `cc1 --dump-ast Tests/invalid_parameter_list.c`.
- Entree : `int broken(,) { }`.
- Observe : segmentation fault, aucun diagnostic source.
- Attendu : erreur interne du frontend rendue par DiagnosticEngine, retour 1.
- Cause : `Parser::parseFuncType()` utilise `ptype->quals` apres que
  `parseTypeSpec()` a retourne `nullptr` sur la virgule.
- Source : `src/parser/Parser.cpp`, ligne 396.
- Regression : `cli_ast_invalid_parameter_list`, execute dans un processus
  separe avec timeout pour ne pas interrompre tous les tests.

### 2. Arguments d'appel regroupes a tort

- Reproduction : `Tests/ast_calls.c`, `sum(1, 2)`.
- Observe : `CallExpr::args` contient un seul `BinaryExpr COMMA`.
- Attendu : deux arguments, les litteraux 1 et 2.
- Cause : `parsePostfix()` appelle `parseExpr()` avec la priorite minimale 0,
  qui absorbe la virgule separant les arguments.
- Source : `src/parser/Pratt.cpp`, ligne 231.
- Regression en echec : `parses_two_call_arguments`.
- Cas de protection qui passe : `identity((1, 2))` doit conserver un seul
  argument contenant l'operateur virgule (`preserves_parenthesized_comma_argument`).

### 3. Noms des parametres perdus

- Reproduction : `Tests/ast_assignments.c`, `assign(int a, int b, int c)`.
- Observe : `Type::params` contient les trois types, mais `FuncDecl::params`
  est vide ; les noms a, b et c ne sont pas conserves.
- Cause : `parseFuncType()` lit `pname` puis le jette ; `parseFuncDecl()` ne
  construit aucun `ParamDecl`.
- Source : `src/parser/Parser.cpp`, lignes 397 et 408.
- Regression en echec : `preserves_function_parameter_names`.

### 4. Declarations de tags absentes de l'AST

- Reproduction : `Tests/mixed_structs.c`.
- Observe : trois `VarDecl`, au lieu des six declarations presentes dans la source.
- Manquent : `struct Foo;`, `union Bar;`, `enum Color;`.
- Cause : `parseDecl()` retourne `nullptr` lorsque le nom du declarateur est
  vide, sans construire le noeud de declaration de tag.
- Source : `src/parser/Parser.cpp`, ligne 134.
- Regression en echec : `preserves_tag_declarations`.

### 5. Valeur des chaines echappees incorrecte

- Reproduction : `Tests/ast_literals.c`, `"hello\n"`.
- Observe : sept octets dans `StringLitExpr::val` : hello, antislash, saut de ligne.
- Attendu : six octets : hello puis saut de ligne.
- Cause : `Lexer::lexString()` ajoute l'antislash a `s` avant d'ajouter le
  caractere decode. Le texte brut et la valeur decodee sont melanges.
- Source : `src/lexer/Lexer.cpp`, ligne 227.
- Regression en echec : `decodes_string_literal_escape`.

### 6. Dump incomplet pour huit categories de noeuds

Le parser construit ces noeuds mais `ASTDump.cpp` imprime `UnsupportedNode`
sans leurs enfants :

- UnaryExpr : `ast_unary.c`.
- TernaryExpr : `ast_conditional.c`.
- CallExpr : `ast_calls.c`, `ast_comma_argument.c`.
- IndexExpr : `ast_index.c`.
- MemberExpr : `ast_member.c`.
- SizeofExpr : `ast_sizeof.c`.
- CharLitExpr et StringLitExpr : `ast_literals.c`.

Regression en echec : `dumps_parsed_expression_nodes`. Les dumps actuels
masquent ainsi le defaut des arguments d'appel et celui des valeurs de chaines.

## Limites actuellement reproduites

- `ast_cast.c` : cast `(int)value` non implemente ; diagnostic a 2:11.
  Une seconde erreur `expected ')' after expression` est aussi emise au meme
  endroit, alors que le premier diagnostic a deja signale l'erreur.
- `ast_local_variable.c` : declaration locale avec initialisation rejetee a 2:3.
- `ast_if.c` : instruction `if` rejetee a 2:3.
- `test_base1.c` : `#include` rejete a 1:1 ; le preprocesseur manque. Son `if`
  et sa boucle `for` ne sont donc pas atteints lors de cette execution.

Ces cas ciblent le frontend syntaxique. Ils ne constituent pas une validation
de l'analyse semantique, qui reste a implementer. En particulier, `ast_member.c`
utilise une declaration de struct incomplete pour exercer le parsing de `->` ;
il faudrait sa definition pour valider l'acces au membre semantiquement.

## Reproduire

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j 2
ctest --test-dir build --output-on-failure
cmake --build build --target ast_audit
```

`ast_audit` collecte toutes les sorties et continue apres un echec. Son succes
signifie que le rapport a ete genere, pas que le compilateur passe tous les cas.
CTest reste en echec tant que les regressions confirmees ne sont pas corrigees.

Ordre de correction recommande : crash, arguments d'appel, chaines echappees,
parametres et declarations de tags, puis couverture du dump. Les constructions
encore non implementees viennent ensuite.
