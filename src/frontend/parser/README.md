# Parser Subsystem

The parser transforms the lexer's flat token stream into an AST for a complete
translation unit. It is a hand-written recursive-descent parser implemented in
C++, with one shared `Parser` object whose methods are split across multiple
`.cpp` files.

The parser primarily targets C, but it also carries a small C++-subset surface
used elsewhere in this repository, including template-related and
`consteval`-related syntax paths selected through `SourceProfile`.

---

## Table of Contents

1. [Why Recursive Descent](#why-recursive-descent)
2. [Module Organization](#module-organization)
3. [The Parser Class](#the-parser-class)
4. [Expression Parsing](#expression-parsing)
5. [Typedef Disambiguation](#typedef-disambiguation)
6. [Declarators and Type Parsing](#declarators-and-type-parsing)
7. [Pragmas as Tokens](#pragmas-as-tokens)
8. [AST Construction and Ownership](#ast-construction-and-ownership)
9. [Error Handling Strategy](#error-handling-strategy)
10. [Where To Edit](#where-to-edit)
11. [Invariants](#invariants)

---

## Why Recursive Descent

This parser is hand-written rather than generated from a grammar because the
language surface here is context-sensitive and extension-heavy:

- C typedef names change how later identifiers are parsed.
- Cast-vs-expression and declaration-vs-expression cases need speculative
  parsing and local backtracking.
- GCC-style extensions such as `__attribute__`, `typeof`, `asm`, and pragma
  side effects are easier to add incrementally in normal C++ code.
- The parser needs direct access to mutable parse-time state such as typedef
  tables, enum constants, `#pragma pack`, and visibility stacks.

The result is a single parser that owns the token cursor and all parse-time
context, rather than a family of smaller generated parsers.

## Module Organization

The `Parser` class is declared once in `parser.hpp`, but its methods are split
across several implementation files:

```txt
parser/
  parser.hpp          -- Parser class, shared state, method declarations
  parser_internal.hpp -- Shared helper declarations used across parser .cpp files
  parse.cpp           -- constructor, token cursor helpers, parse() entry point, AST dump
  types.cpp           -- type specifiers, declarators, struct/union/enum parsing
  expressions.cpp     -- expression parsing, precedence handling, postfix/unary parsing
  statements.cpp      -- statement forms and block parsing
  declarations.cpp    -- local/global declarations and top-level item parsing
  common.cpp          -- shared helpers for constant evaluation and type checks
  ast.hpp             -- AST node definitions and TypeSpec data structures
```

Even though the implementation is physically split, there is still exactly one
parser state machine.

## The Parser Class

The main parser state lives on `c4c::Parser` in `parser.hpp`:

- `tokens_`, `pos_`: the full token stream and current read position
- `arena_`: arena allocator used for all AST node and string-backed storage
- `source_profile_`: controls profile-specific syntax behavior
- `typedefs_`, `user_typedefs_`, `typedef_types_`: typedef tracking and
  resolution
- `typedef_fn_ptr_info_`: cached function-pointer typedef metadata
- `struct_defs_`, `defined_struct_tags_`, `struct_tag_def_map_`: tag/body
  bookkeeping for structs, unions, and enums
- `enum_consts_`: parse-time enum constant values
- `var_types_`: variable-type table used by constructs such as `typeof`
- `pack_alignment_`, `pack_stack_`: `#pragma pack` state
- `visibility_`, `visibility_stack_`: `#pragma GCC visibility` state
- `had_error_`: sticky parse failure flag for recoverable-error paths

This design keeps all ambiguity-resolution and parse-time semantic hints local
to one object, which is why helper functions are methods on `Parser` instead of
free functions.

## Expression Parsing

Expressions are parsed with a Pratt / precedence-climbing style split across:

- `parse_expr()` for comma-level expressions
- `parse_assign_expr()` for assignment and ternary-adjacent logic
- `parse_binary(min_prec)` for precedence-based binary parsing
- `parse_unary()`, `parse_postfix()`, and `parse_primary()` for tighter forms

This keeps precedence logic centralized while still allowing custom handling for
casts, postfix operators, calls, subscripting, member access, and initializer
contexts.

The parser relies on token lookahead rather than token pushback streams. Since
the full token vector is already materialized, speculative parsing is done by
saving and restoring `pos_` when necessary.

## Typedef Disambiguation

C parsing is context-sensitive because identifiers can be either names or
types. This parser resolves that ambiguity with live parser state:

- `typedefs_` answers "is this identifier currently a type name?"
- `typedef_types_` maps typedef names to resolved `TypeSpec`
- `user_typedefs_` distinguishes locally introduced typedefs from seeded names
- `last_resolved_typedef_` helps declarator parsing propagate function-pointer
  typedef metadata

This matters in ambiguous constructs such as:

```c
T *x;
T(y);
sizeof(T);
```

Whether these are declarations, expressions, or type names depends on the
typedef table at that exact parse point.

## Declarators and Type Parsing

`types.cpp` handles the hardest part of the C grammar:

- base type-specifier collection
- qualifiers and attributes
- pointers, arrays, and function declarators
- struct / union / enum definitions and tag references
- abstract type names used by casts, `sizeof`, and similar constructs

The parser keeps base-type parsing and declarator parsing distinct:

- `parse_base_type()` determines the underlying type and qualifiers
- `parse_declarator()` applies pointer / array / function shape and extracts the
  declared name
- `parse_type_name()` parses unnamed type forms used in expression contexts

This split is the key to keeping inside-out declarator syntax manageable.

## Pragmas as Tokens

Some pragma semantics intentionally cross the preprocessor/parser boundary.
The preprocessor preserves certain directives as text, the lexer turns them into
synthetic tokens, and the parser consumes those tokens as structured state
updates.

The currently supported parser-level pragma flows are:

- `#pragma pack(...)`
- `#pragma weak ...`
- `#pragma GCC visibility push/pop`

The parser handles these through dedicated state fields and helpers such as
`handle_pragma_pack()` and `handle_pragma_gcc_visibility()`.

## AST Construction and Ownership

All AST objects are arena-allocated.

- `make_node()` and the other `make_*` helpers allocate nodes from `arena_`
- parsed top-level items are gathered into a translation-unit `NK_PROGRAM`
- supporting tag definitions may be collected in `struct_defs_` and injected so
  downstream lowering/codegen sees type information before dependent uses

This keeps ownership simple: the parser never frees individual nodes, and the
whole AST lives for the lifetime of the arena.

## Error Handling Strategy

This parser uses a mixed strategy:

- hard failures through `expect(...)` when a construct cannot continue
- local recovery helpers such as `skip_until(...)` for statement/declaration
  boundaries
- sticky parser state via `had_error_` so the driver can stop after parsing

It is not a "maximal recovery" parser. The goal is to recover enough to report
useful diagnostics and avoid cascading nonsense, not to preserve a perfectly
well-formed partial tree after arbitrary syntax damage.

## Where To Edit

- Add or change expression syntax: `expressions.cpp`
- Add or change statement syntax: `statements.cpp`
- Add new type keywords, declarators, or tag behavior: `types.cpp`
- Change top-level or local declaration behavior: `declarations.cpp`
- Change shared constant-eval or compatibility helpers: `common.cpp`
- Change parser-wide state or public method declarations: `parser.hpp`

## Invariants

- There is one parser object and one token cursor; do not split state across
  multiple parser instances.
- `parse.cpp` remains the single `Parser::parse()` entry point.
- Shared helpers used from multiple parser modules belong in `common.cpp` with
  declarations kept in `parser_internal.hpp`.
- AST allocation must remain arena-based.
- Typedef, pragma-pack, and visibility state are parse-time state, not deferred
  semantic passes.
