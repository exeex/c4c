# Parser Agent Index Header Hierarchy Runbook

Status: Active
Source Idea: ideas/open/92_parser_agent_index_header_hierarchy.md

## Purpose

Make `src/frontend/parser` headers communicate semantic visibility by path
depth: top-level headers are public parser/AST surfaces, and parser-private
implementation indexes live under `src/frontend/parser/impl/`.

## Goal

Shrink `parser.hpp` into the public parser entry surface while creating a
private parser implementation index for the split parser `.cpp` files.

## Core Rule

This is a structural header-boundary refactor. Preserve parser behavior,
AST output, testcase expectations, and visible-name semantics.

## Read First

- `ideas/open/92_parser_agent_index_header_hierarchy.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/types_helpers.hpp`
- parser implementation files under `src/frontend/parser/parser_*.cpp`

## Current Targets

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

## Non-Goals

- Do not split or move `ast.hpp`.
- Do not introduce a traditional separated `include/` tree.
- Do not create one header per parser `.cpp` file.
- Do not move parser `.cpp` files unless a later step proves it is necessary.
- Do not rename directories around implementation algorithm names such as
  `recursive_descent/`.
- Do not change parser semantics or downgrade expectations as proof.
- Do not absorb HIR header hierarchy work from idea 93.

## Working Model

- `src/frontend/parser/*.hpp` is public by default.
- `src/frontend/parser/impl/*.hpp` is private to parser implementation files.
- `src/frontend/parser/impl/types/*.hpp` is private to type,
  declarator, struct, and template parsing internals.
- `parser.hpp` should expose only the constructor, `parse()`, and public result
  or option types that external callers need.
- `impl/parser_impl.hpp` should become the private navigation index for
  parser internals and split translation-unit method declarations.

## Execution Rules

- Keep slices behavior-preserving and compile after each code-changing step.
- Prefer compatibility includes only as an incremental bridge, and remove them
  once callers are retargeted.
- Move declarations only when their new visibility boundary is clear.
- Keep AST and `TypeSpec` declarations centralized in `ast.hpp`.
- If semantic cleanup is discovered, record it as a separate idea unless it is
  required to keep the structural refactor compiling.
- For code-changing steps, run:
  `cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- Escalate validation if a slice changes AST-facing declarations,
  HIR-facing include paths, app entry includes, or shared frontend
  infrastructure.

## Step 1: Inventory Parser Header Boundaries

Goal: classify the current public/private parser header surface before moving
declarations.

Primary target: `src/frontend/parser/parser.hpp`,
`src/frontend/parser/parser_state.hpp`, `src/frontend/parser/types_helpers.hpp`,
and current include sites.

Concrete actions:

- Inspect declarations in `parser.hpp` and group them as public parser API,
  parser-private state, split-implementation method declarations, debug
  plumbing, lookup tables, template/type helpers, or AST-facing data.
- Inspect include sites for `parser.hpp`, `parser_state.hpp`, and
  `types_helpers.hpp` and identify which are external callers versus parser
  implementation files.
- Record the proposed first movement boundary in `todo.md`; do not move files
  or rewrite includes in this step unless a trivial correction is required for
  plan accuracy.

Completion check:

- `todo.md` names the first code-changing sub-slice and any include-site
  hazards.
- No build proof is required if this remains inventory-only.

## Step 2: Introduce Private Parser Implementation Index

Goal: create `src/frontend/parser/impl/parser_impl.hpp` and route parser
implementation files through it.

Primary target: `parser.hpp`, new `impl/parser_impl.hpp`, parser `.cpp`
includes.

Concrete actions:

- Add `impl/parser_impl.hpp` as the private implementation index.
- Move private parser state aliases, helper declarations, debug plumbing, and
  split translation-unit method declarations out of the public header where
  possible.
- Update parser implementation `.cpp` files to include the private index.
- Keep `parser.hpp` usable by external callers that construct `Parser` and call
  `parse()`.

Completion check:

- External users can include `parser.hpp` without depending on the full private
  parser implementation index.
- Parser implementation files compile through `impl/parser_impl.hpp`.
- Focused parser validation passes.

## Step 3: Move Parser State Support Under `impl/`

Goal: make parser-private state carriers live under the private implementation
boundary.

Primary target: `parser_state.hpp` to `impl/parser_state.hpp` and affected
includes.

Concrete actions:

- Relocate parser-private state declarations to `src/frontend/parser/impl/`.
- Retarget private include sites to the new path.
- Keep a compatibility include only if required for an incremental slice, and
  remove it before closing this idea if no external caller needs it.

Completion check:

- Top-level parser headers no longer expose parser-private state carriers by
  default.
- Focused parser validation passes.

## Step 4: Move Type Parsing Helpers Under `impl/types/`

Goal: give type/declarator/template parsing helpers a private subdomain index.

Primary target: `types_helpers.hpp`, optional new `impl/types/types.hpp`, and
type parser `.cpp` includes.

Concrete actions:

- Move `types_helpers.hpp` under `src/frontend/parser/impl/types/`.
- Add `impl/types/types.hpp` only if there are meaningful shared
  type/declarator/template declarations to index.
- Do not create one-off headers such as `declarator.hpp`, `template.hpp`, or
  `struct_types.hpp` unless a real subdirectory-level boundary emerges.

Completion check:

- Type/declarator/template parser implementation files use the private type
  helper path.
- Top-level parser headers remain public surfaces by default.
- Focused parser validation passes.

## Step 5: Retarget External Include Sites And Remove Bridges

Goal: finish the public/private parser header boundary and remove temporary
compatibility includes.

Primary target: external parser callers, parser tests, and remaining top-level
private headers.

Concrete actions:

- Retarget external callers to include only public parser headers they need.
- Remove compatibility includes once all callers have moved.
- Confirm the number of thin single-purpose top-level parser headers decreases
  or stays flat.
- Check for stale references to private headers from outside parser
  implementation code.

Completion check:

- `parser.hpp` is the public parser entry API.
- Parser-private headers live under `src/frontend/parser/impl/` by default.
- Focused parser validation passes.
- Escalated validation has been run if non-parser frontend or app include
  paths changed.

## Step 6: Closure Validation

Goal: prove the structural refactor is complete enough to close idea 92 or
identify a follow-on split.

Concrete actions:

- Re-read the source idea acceptance criteria.
- Check that remaining top-level parser headers are intentionally public.
- Check that private parser and type parsing indexes are discoverable under
  `impl/`.
- Run the validation level selected by the supervisor for closure.

Completion check:

- Source idea acceptance criteria are met, or unresolved work is recorded as a
  separate open idea before closing this active plan.
