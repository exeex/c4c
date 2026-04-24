# Parser Agent Index Header Hierarchy Runbook

Status: Active
Source Idea: ideas/open/92_parser_agent_index_header_hierarchy.md

## Purpose

Finish the parser directory hierarchy refactor so path depth communicates
public parser API, parser-private implementation, and type/declarator/template
subdomain ownership.

Goal: parser implementation files live under semantic `impl/` directories with
redundant filename prefixes removed, while public parser headers stay small and
behavior stays unchanged.

## Core Rule

`directory = semantic boundary`; once parser implementation files live under
`impl/`, the directory carries the parser context and filenames should be short.

## Read First

- `ideas/open/92_parser_agent_index_header_hierarchy.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_impl.hpp`
- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/impl/types/types_helpers.hpp`
- parser implementation files under `src/frontend/parser/`
- parser CMake/source-list declarations

## Current Targets

- Public parser surfaces:
  - `src/frontend/parser/ast.hpp`
  - `src/frontend/parser/parser.hpp`
  - `src/frontend/parser/parser_support.hpp`
  - `src/frontend/parser/parser_types.hpp`
- Private parser implementation boundary:
  - `src/frontend/parser/impl/parser_impl.hpp`
  - `src/frontend/parser/impl/parser_state.hpp`
- Type parsing subdomain boundary:
  - `src/frontend/parser/impl/types/types_helpers.hpp`
  - optional `src/frontend/parser/impl/types/types.hpp` only if useful shared
    declarations remain after movement
- Implementation files to move:
  - `parser_core.cpp` -> `impl/core.cpp`
  - `parser_declarations.cpp` -> `impl/declarations.cpp`
  - `parser_expressions.cpp` -> `impl/expressions.cpp`
  - `parser_statements.cpp` -> `impl/statements.cpp`
  - `parser_support.cpp` -> `impl/support.cpp`
  - `parser_types_base.cpp` -> `impl/types/base.cpp`
  - `parser_types_declarator.cpp` -> `impl/types/declarator.cpp`
  - `parser_types_struct.cpp` -> `impl/types/struct.cpp`
  - `parser_types_template.cpp` -> `impl/types/template.cpp`

## Non-Goals

- Do not change parser semantics, AST output, diagnostics behavior, visible-name
  behavior, pragma behavior, template parsing behavior, or testcase
  expectations.
- Do not weaken tests or downgrade supported paths as proof.
- Do not split `ast.hpp` as part of this structural parser idea.
- Do not create a traditional separated `include/` tree.
- Do not introduce one header per parser implementation file.
- Do not rename directories around implementation algorithms such as
  `recursive_descent/`.
- Do not mix this work with semantic parser lookup-result redesigns or HIR
  hierarchy movement.

## Working Model

The target parser layout is:

```text
src/frontend/parser/
  ast.hpp
  parser.hpp
  parser_support.hpp
  parser_types.hpp

  impl/
    parser_impl.hpp
    parser_state.hpp
    core.cpp
    declarations.cpp
    expressions.cpp
    statements.cpp
    support.cpp

    types/
      types_helpers.hpp
      base.cpp
      declarator.cpp
      struct.cpp
      template.cpp
```

Top-level parser headers are public or facade-facing by default. Nested
`impl/` files are private to parser implementation unless explicitly used by
parser tests for named private hooks.

## Execution Rules

- Keep each code slice behavior-preserving and buildable.
- Move one file family at a time when possible.
- Update build-system source lists in the same slice as each file movement.
- Retarget relative includes after movement; keep implementation files using
  `impl/parser_impl.hpp` or the local private index, not public headers as a
  substitute for private declarations.
- Avoid compatibility include shims unless required for a narrow incremental
  slice, and remove them before close.
- After each code-changing step, run at least:
  - `cmake --build build -j --target c4c_frontend c4cll`
  - `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- Escalate validation if a slice changes AST-facing declarations, HIR-facing
  include paths, app entry includes, or shared frontend infrastructure.

## Step 1: Inventory Parser Layout And Build References

Goal: identify every current parser implementation path, include path, and
build-system source-list reference that must move together.

Primary targets:
- `src/frontend/parser/`
- parser-related CMake/source-list declarations
- parser tests and app/frontend include sites

Actions:
- Record the current parser file tree and distinguish public headers,
  private headers, top-level implementation files, and type-family
  implementation files.
- Find all references to the current top-level implementation filenames in
  build files, scripts, docs, tests, and source includes.
- Verify that `parser.hpp` remains a public facade and that implementation
  files already have a usable private index.

Completion check:
- The concrete move set and build/source reference list are recorded in
  `todo.md`.
- No implementation files are changed unless needed for a compile probe.

## Step 2: Move Core Parser Implementation Files Under `impl/`

Goal: move non-type parser implementation files into
`src/frontend/parser/impl/` and remove redundant `parser_` filename prefixes.

Primary targets:
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_expressions.cpp`
- `src/frontend/parser/parser_statements.cpp`
- `src/frontend/parser/parser_support.cpp`
- build-system source lists that name these files

Actions:
- Rename the five core parser implementation files to
  `impl/core.cpp`, `impl/declarations.cpp`, `impl/expressions.cpp`,
  `impl/statements.cpp`, and `impl/support.cpp`.
- Update includes made relative by the new directory depth.
- Update build-system source paths in the same slice.
- Keep private implementation declarations behind `impl/parser_impl.hpp`.

Completion check:
- The five core parser implementation files no longer remain flat at
  top-level.
- `c4c_frontend`, `c4cll`, and focused parser tests build/pass.

## Step 3: Move Type Parser Implementation Files Under `impl/types/`

Goal: move type/declarator/template parser implementation files into the type
subdomain directory and remove redundant `parser_types_` filename prefixes.

Primary targets:
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_declarator.cpp`
- `src/frontend/parser/parser_types_struct.cpp`
- `src/frontend/parser/parser_types_template.cpp`
- `src/frontend/parser/impl/types/types_helpers.hpp`
- build-system source lists that name these files

Actions:
- Rename the type-family implementation files to
  `impl/types/base.cpp`, `impl/types/declarator.cpp`,
  `impl/types/struct.cpp`, and `impl/types/template.cpp`.
- Retarget relative includes for the additional directory depth.
- Add `impl/types/types.hpp` only if it becomes a meaningful shared type
  parsing subdomain index; do not add it as a thin wrapper.
- Update build-system source paths in the same slice.

Completion check:
- The four type-family implementation files live under `impl/types/`.
- Agents editing type/declarator/template parsing have an obvious subdomain
  index.
- `c4c_frontend`, `c4cll`, and focused parser tests build/pass.

## Step 4: Clean Compatibility Fallout And Documentation

Goal: remove temporary structural fallout and make the final parser directory
shape self-explanatory.

Primary targets:
- parser include sites touched during movement
- parser boundary or README/audit docs if they still name old paths
- stale build, script, or test references to old top-level implementation
  filenames

Actions:
- Search for old `parser_*.cpp` and `parser_types_*.cpp` path references and
  remove or update them when they are no longer accurate.
- Remove temporary compatibility includes or stale comments introduced during
  movement.
- Refresh parser-local documentation only where it describes file layout or
  agent navigation.

Completion check:
- No stale source/build/doc references imply that moved parser implementation
  files still live at top-level.
- Top-level parser headers remain public surfaces by default.
- Parser-private headers and implementation files live under `impl/` by
  default.

## Step 5: Final Parser Hierarchy Validation

Goal: prove the structural migration is complete and behavior-preserving.

Primary targets:
- full parser file tree
- parser build/test proof
- broader frontend proof if include-path blast radius justifies it

Actions:
- Verify the target file tree exists with no remaining top-level parser
  implementation `.cpp` files from the move set.
- Verify `parser.hpp` does not regain private parser-state includes.
- Run the standard parser proof:
  - `cmake --build build -j --target c4c_frontend c4cll`
  - `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- Escalate to broader or full validation if build-system or shared include
  changes affected more than parser implementation paths.

Completion check:
- Acceptance criteria from the source idea are satisfied.
- Final proof is recorded in `todo.md`.
- The plan owner can decide whether the source idea is close-ready.
