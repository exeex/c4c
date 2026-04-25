# Parser Agent Index Header Hierarchy

Status: Closed
Created: 2026-04-24
Last Updated: 2026-04-24
Parent Ideas:
- [87_parser_visible_name_resolution_structured_result.md](/workspaces/c4c/ideas/closed/87_parser_visible_name_resolution_structured_result.md)

Reopened: 2026-04-24

## Completion Notes

Closed after the reopened parser hierarchy runbook completed. Core parser
implementation files now live under `src/frontend/parser/impl/` with short
filenames, and type/declarator/template implementation files now live under
`src/frontend/parser/impl/types/` with short filenames. `parser.hpp` remains
the public facade, parser-private headers live under `impl/`, and the type
subdomain keeps `types_helpers.hpp` as its private index; no thin
`impl/types/types.hpp` wrapper was added because no meaningful shared index
declarations remained.

Focused parser proof passed, the full CTest suite passed `2974/2974`, and the
close-time regression guard passed with no new failures.

## Reopen Note

The earlier structural pass moved parser-private headers under `impl/`, but it
did not move the parser implementation `.cpp` files into the semantic
directories. That is not enough for the intended agent workflow. The source
tree itself should show where an agent should work, and moving files should
also remove repeated filename prefixes such as `parser_` and `parser_types_`.

The required shape is:

```text
src/frontend/parser/
  ast.hpp
  parser.hpp
  parser_support.hpp

  impl/
    parser_impl.hpp
    parser_state.hpp

    core.cpp
    declarations.cpp
    expressions.cpp
    statements.cpp
    support.cpp

    types/
      types.hpp
      types_helpers.hpp
      base.cpp
      declarator.cpp
      struct.cpp
      template.cpp
```

The top-level `parser_*.cpp` and `parser_types_*.cpp` prefixes are not the
desired final agent surface. Once a file lives under a semantic directory, the
directory should carry that context and the filename should be short.

## Goal

Optimize `src/frontend/parser` for LLM-agent programming by treating headers
as semantic index surfaces instead of one-to-one implementation partners.

The parser should expose a small public parsing surface to external callers,
while agents working inside the parser should have one obvious private
implementation index to open before editing parser internals.

## Why This Idea Exists

The parser already follows a split implementation style:

- `parser_core.cpp`
- `parser_declarations.cpp`
- `parser_expressions.cpp`
- `parser_statements.cpp`
- `parser_types_base.cpp`
- `parser_types_declarator.cpp`
- `parser_types_struct.cpp`
- `parser_types_template.cpp`
- `parser_support.cpp`

However, the current header layout does not fully communicate visibility by
path depth. `parser.hpp` acts as both the external parser API and the private
implementation index, while `parser_state.hpp` and `types_helpers.hpp` sit at
the top-level parser directory even though they are mostly parser-private
implementation support.

This creates unnecessary context load for agents and external parser callers.
An agent opening `parser.hpp` should quickly understand the public parser
entry surface. An agent editing parser internals should have a private
implementation index that describes the implementation boundary and the
shared declarations used by the split parser `.cpp` files.

## Design Direction

Use this rule:

```text
directory = semantic boundary
.hpp      = index entry for that boundary
```

For the parser, the preferred visibility shape is:

```text
src/frontend/parser/*.hpp
  public frontend parser/AST surfaces

src/frontend/parser/impl/*.hpp
  private to parser implementation files

src/frontend/parser/impl/types/*.hpp
  private to parser type/declarator/template parsing internals
```

The directory names should describe responsibility and visibility, not the
parser algorithm. Avoid names such as `recursive_descent/`; they overfit the
current implementation style and do not communicate a durable semantic
boundary. Prefer `impl/` for parser-private machinery and `impl/types/` for
the type/declarator/template subdomain.

## Target Shape

Preferred long-term layout:

```text
src/frontend/parser/
  ast.hpp
  parser.hpp
  parser_support.hpp

  impl/
    parser_impl.hpp
    parser_state.hpp

    core.cpp
    declarations.cpp
    expressions.cpp
    statements.cpp
    support.cpp

    types/
      types.hpp
      types_helpers.hpp
      base.cpp
      declarator.cpp
      struct.cpp
      template.cpp
```

The key requirement is not merely adding private index headers. Parser
implementation `.cpp` files should move into the semantic `impl/` subdirectory
that owns them, and their filenames should drop duplicated prefixes that the
directory path already communicates:

```text
parser_core.cpp              -> impl/core.cpp
parser_declarations.cpp      -> impl/declarations.cpp
parser_expressions.cpp       -> impl/expressions.cpp
parser_statements.cpp        -> impl/statements.cpp
parser_support.cpp           -> impl/support.cpp
parser_types_base.cpp        -> impl/types/base.cpp
parser_types_declarator.cpp  -> impl/types/declarator.cpp
parser_types_struct.cpp      -> impl/types/struct.cpp
parser_types_template.cpp    -> impl/types/template.cpp
```

The first reopened slice may move one family at a time, but the acceptance
target is the file-tree migration, not only header visibility.

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/types_helpers.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_expressions.cpp`
- `src/frontend/parser/parser_statements.cpp`
- `src/frontend/parser/parser_support.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_declarator.cpp`
- `src/frontend/parser/parser_types_struct.cpp`
- `src/frontend/parser/parser_types_template.cpp`
- focused include-site updates in parser tests and frontend/app callers

## Refactoring Steps

1. Keep the public AST model centralized in `ast.hpp`.
   - `ast.hpp` is not parser-private. HIR, sema, codegen, tests, and apps all
     consume AST and `TypeSpec` data model declarations.
   - Do not move or split AST declarations merely to make parser headers look
     symmetric with backend BIR headers.

2. Shrink `parser.hpp` into a public parser entry header.
   - Keep only the declarations external callers need to construct and invoke
     the parser, plus any small public result or option types.
   - Remove private parser state carriers, implementation helper families, and
     split-translation-unit declarations from the public surface.

3. Introduce `src/frontend/parser/impl/parser_impl.hpp`.
   - Make this the private implementation index for `parser_*.cpp`.
   - Move private parser context, state aliases, helper declarations, debug
     plumbing, and split implementation method declarations behind this
     boundary.
   - Update parser implementation `.cpp` files to include the private index
     instead of relying on the public `parser.hpp` as their full internal map.
   - Move non-type parser implementation `.cpp` files under
     `src/frontend/parser/impl/` and remove the duplicated `parser_` prefix:
     `parser_core.cpp` -> `impl/core.cpp`,
     `parser_declarations.cpp` -> `impl/declarations.cpp`,
     `parser_expressions.cpp` -> `impl/expressions.cpp`,
     `parser_statements.cpp` -> `impl/statements.cpp`, and
     `parser_support.cpp` -> `impl/support.cpp`.

4. Move parser state support under `impl/`.
   - Relocate `parser_state.hpp` to `impl/parser_state.hpp` once the public
     header no longer needs to expose those declarations directly.
   - Keep compatibility includes only if required for an incremental slice,
     and remove them when callers have been retargeted.

5. Split the type parsing subdomain only after the public/private parser
   boundary is clean.
   - Move `types_helpers.hpp` under `impl/types/`.
   - Add `impl/types/types.hpp` only if there are meaningful shared
     type/declarator/template declarations to index.
   - Move type-family `.cpp` files under `src/frontend/parser/impl/types/`
     and remove the duplicated `parser_types_` prefix:
     `parser_types_base.cpp` -> `impl/types/base.cpp`,
     `parser_types_declarator.cpp` -> `impl/types/declarator.cpp`,
     `parser_types_struct.cpp` -> `impl/types/struct.cpp`, and
     `parser_types_template.cpp` -> `impl/types/template.cpp`.
   - Do not create one-off headers such as `declarator.hpp`, `template.hpp`,
     or `struct_types.hpp` unless a real subdirectory-level boundary emerges.

6. Preserve source-level behavior.
   - This initiative is structural.
   - Do not change parser semantics, testcase expectations, AST output, or
     visible-name behavior as part of the layout refactor.
   - Semantic cleanup discovered during the move should become a separate
     idea unless it is required to keep the structural change compiling.

## Acceptance Criteria

- External users can include `src/frontend/parser/parser.hpp` without pulling
  in the full private parser implementation state and helper declarations.
- Agents working inside `src/frontend/parser/` can open
  `impl/parser_impl.hpp` as the private parser implementation index.
- Core parser implementation `.cpp` files live under
  `src/frontend/parser/impl/` with redundant `parser_` prefixes removed.
- Agents working on type/declarator/template parsing can use
  `impl/types/types.hpp` or `impl/types/types_helpers.hpp` as the type parsing
  subdomain index instead of guessing among unrelated parser files.
- Type-family parser implementation `.cpp` files live under
  `src/frontend/parser/impl/types/` with redundant `parser_types_` prefixes
  removed.
- Top-level parser headers represent public surfaces by default.
- Parser-private headers live under `src/frontend/parser/impl/` by default.
- Parser implementation `.cpp` files do not remain flat at top-level after
  their semantic `impl/` directory exists.
- The number of thin single-purpose headers in `src/frontend/parser` decreases
  or stays flat.
- Frontend parser tests continue to pass.
- `c4c_frontend` and `c4cll` build after each structural slice.

## Validation

At minimum:

- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Escalate validation if the slice changes AST-facing declarations, HIR-facing
include paths, app entry includes, or shared frontend infrastructure.

## Non-Goals

- no traditional separated `include/` tree
- no one-header-per-parser-`.cpp` convention
- no parser semantic changes
- no expectation downgrades as proof
- no AST model split unless a separate frontend-wide AST initiative is opened
- no renaming to algorithm-centric directories such as `recursive_descent/`
- no keeping implementation `.cpp` files at top level once their semantic
  `impl/` directory exists

## Relationship To Other Parser Work

This idea is structural and should not absorb the semantic parser identity
work in:

- [87_parser_visible_name_resolution_structured_result.md](/workspaces/c4c/ideas/closed/87_parser_visible_name_resolution_structured_result.md)

If both ideas are active candidates, prefer not to mix semantic lookup
representation changes with header hierarchy movement in the same code slice.
