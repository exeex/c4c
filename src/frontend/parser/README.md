# Parser Subsystem

This directory contains the recursive-descent parser for `tiny-c2ll`.
It consumes lexer tokens and produces AST nodes for a full translation unit.

## Module Layout

```txt
parser/
  parser.hpp          # Parser class interface (all parser methods declared here)
  parser_internal.hpp # Shared helper function declarations used across parser .cpp files
  parse.cpp           # Parser constructor, token cursor helpers, parse() entry, AST dump helpers
  types.cpp           # Type-specifier/declarator parsing, struct/union/enum parsing, parameter parsing
  expressions.cpp     # Pratt/precedence expression parsing, primary/postfix/unary, initializer parsing
  statements.cpp      # Statement parsing (block/if/for/while/switch/return/etc.)
  declarations.cpp    # Local/global declaration parsing and top-level item parsing
  common.cpp          # Shared constant/type helper implementations
  ast.hpp             # AST definitions
```

## Single Parser State

Even though implementation is split across files, there is still one `Parser` state:

- `tokens_`, `pos_`: token stream and cursor
- `typedefs_`, `typedef_types_`: typedef disambiguation and type resolution
- `struct_defs_`, `struct_tag_def_map_`: parsed struct/enum defs and lookup maps
- `enum_consts_`, `var_types_`: constant-expression and variable type tracking

All module files implement methods on the same `Parser` class.

## Read Order For LLM

For quickest onboarding, read in this order:

1. `parser.hpp` (API and parser state fields)
2. `parse.cpp` (entry points and cursor helpers)
3. `types.cpp` (type grammar and declarators)
4. `expressions.cpp` (expression grammar and precedence)
5. `statements.cpp` (control-flow statements)
6. `declarations.cpp` (local/global/top-level declaration flows)
7. `common.cpp` (shared evaluators and compatibility helpers)

## Where To Edit

- Add/adjust operator precedence or expression form:
  - edit `expressions.cpp`
- Add new statement syntax:
  - edit `statements.cpp`
- Add new type keyword / declarator form / tag behavior:
  - edit `types.cpp`
- Change local/global declaration behavior:
  - edit `declarations.cpp`
- Change constant-expression/type-compatibility helper logic:
  - edit `common.cpp` and declarations in `parser_internal.hpp`

## Invariants

- Do not split parser state into multiple parser objects.
- Keep shared helper declarations in `parser_internal.hpp` consistent with `common.cpp`.
- Keep `parse.cpp` as the single `Parser::parse()` entry point.
- If a helper is needed by multiple parser modules, put it in `common.cpp`.
