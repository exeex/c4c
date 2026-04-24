# Parser Public Facade Completion Runbook

Status: Active
Source Idea: ideas/open/92_parser_agent_index_header_hierarchy.md

## Purpose

Finish idea 92 after closure validation found that the directory hierarchy was
moved but the public/private header boundary is not yet honest.

## Goal

Make `src/frontend/parser/parser.hpp` the public parser entry surface, with
parser implementation navigation and private state living behind
`src/frontend/parser/impl/`.

## Core Rule

This remains a structural header-boundary refactor. Preserve parser behavior,
AST output, testcase expectations, and visible-name semantics.

## Closure Finding

Step 6 validation passed, but close was rejected because source-idea acceptance
criteria are not yet met:

- `parser.hpp` still describes itself as the parser implementation navigation
  index.
- `parser.hpp` still exposes parser-private state carriers through
  `#include "impl/parser_state.hpp"` and public data members.
- `impl/parser_impl.hpp` is still a bridge that includes `../parser.hpp`
  instead of being the private implementation index.
- The public header still contains split translation-unit helper declarations
  and debug/recovery/private lookup declarations that external parser callers
  do not need to construct `Parser` and call `parse()`.

## Read First

- `ideas/open/92_parser_agent_index_header_hierarchy.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_impl.hpp`
- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/impl/types/types_helpers.hpp`
- parser implementation files under `src/frontend/parser/parser_*.cpp`

## Current Targets

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_impl.hpp`
- `src/frontend/parser/impl/parser_state.hpp`
- parser implementation files under `src/frontend/parser/parser_*.cpp`
- focused parser/app/test call sites that construct `Parser` or inspect parser
  debug state

## Non-Goals

- Do not split or move `ast.hpp`.
- Do not introduce a traditional separated `include/` tree.
- Do not create one header per parser `.cpp` file.
- Do not move parser `.cpp` files.
- Do not rename implementation directories around parser algorithms.
- Do not change parser semantics or downgrade expectations as proof.
- Do not absorb HIR header hierarchy work from idea 93.

## Working Model

- `src/frontend/parser/parser.hpp` is the public parser entry API.
- `src/frontend/parser/impl/parser_impl.hpp` is the private implementation
  index opened by parser implementation files.
- `src/frontend/parser/impl/parser_state.hpp` holds parser-private state
  carriers.
- `src/frontend/parser/impl/types/types_helpers.hpp` remains private to type,
  declarator, struct, and template parsing internals.

## Execution Rules

- Keep slices behavior-preserving and compile after each code-changing step.
- Prefer small mechanical movement over semantic cleanup.
- If the existing "all members public" constraint prevents moving a member
  declaration directly, either introduce an implementation-owned wrapper/facade
  boundary or record the exact blocker in `todo.md` before widening scope.
- Do not leave `impl/parser_impl.hpp` as a compatibility bridge at closure.
- For code-changing steps, run:
  `cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- Before closing, run the supervisor-selected broader validation and regression
  guard.

## Step 1: Inventory Public Parser Consumers

Goal: determine the true public surface that must remain in `parser.hpp`.

Primary target: external include sites and direct `Parser` member use outside
`src/frontend/parser/parser_*.cpp`.

Concrete actions:

- Inspect non-parser implementation callers that include `parser.hpp`.
- Classify each used `Parser` member as public API, debug/test API, or
  accidental implementation access.
- Record whether public debug/test access must remain in `parser.hpp` or can
  move behind a narrower public support surface.
- Identify parser implementation declarations that can move to
  `impl/parser_impl.hpp` without changing call sites.

Completion check:

- `todo.md` lists the public members that must remain in `parser.hpp`.
- `todo.md` names the first code-changing sub-slice and any facade/wrapper
  blocker.
- No build proof is required if this remains inventory-only.

## Step 2: Move Implementation Navigation Behind `impl/parser_impl.hpp`

Goal: make parser implementation files get their private map from
`impl/parser_impl.hpp`, not from public `parser.hpp`.

Primary target: `parser.hpp`, `impl/parser_impl.hpp`, parser `.cpp` files.

Concrete actions:

- Move declarations and helper APIs used only by parser implementation files
  out of `parser.hpp` when the C++ shape allows it.
- Where member declarations must remain on `Parser`, keep only the minimal
  declaration in `parser.hpp` and move implementation-only aliases,
  commentary, and navigation grouping to `impl/parser_impl.hpp`.
- Remove bridge wording from `impl/parser_impl.hpp`; it must describe the
  private implementation boundary.
- Update parser implementation files if the include order or helper access
  changes.

Completion check:

- Parser implementation files compile through `impl/parser_impl.hpp` as their
  private index.
- `parser.hpp` no longer presents itself as the implementation navigation map.
- Focused parser validation passes.

## Step 3: Minimize Public Exposure Of Parser State

Goal: stop external parser callers from pulling private state definitions
unless the current parser object layout makes that unavoidable.

Primary target: `parser.hpp`, `impl/parser_state.hpp`.

Concrete actions:

- Try to remove `#include "impl/parser_state.hpp"` from `parser.hpp` by
  introducing an implementation-owned state boundary.
- If a full facade/PIMPL split is required, keep the slice small: preserve
  constructor and `parse()` behavior while moving private state ownership out
  of the public header.
- If the project coding constraints make a facade split too large for this
  idea, record the exact constraint and remaining exposure in `todo.md` and
  create a separate open idea instead of closing silently.

Completion check:

- External users can include `parser.hpp` without the full private parser
  implementation state, or a separate open idea records why that requires a
  larger follow-on rebuild.
- Focused parser validation passes.

## Step 4: Closure Validation

Goal: prove the source idea is complete or split the unresolved public facade
work before closing.

Concrete actions:

- Re-read the source idea acceptance criteria.
- Check top-level parser headers are intentionally public.
- Check `impl/parser_impl.hpp` is a real private index, not just a bridge.
- Check stale top-level private headers and compatibility includes are absent.
- Run the validation level selected by the supervisor.

Completion check:

- Source idea acceptance criteria are met, or unresolved work is recorded as a
  separate open idea before closing this active plan.
