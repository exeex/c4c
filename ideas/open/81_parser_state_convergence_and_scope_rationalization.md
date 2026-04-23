# Parser State Convergence And Scope Rationalization

Status: Open
Last Updated: 2026-04-23

## Goal

Continue the new `parser_state.hpp` split until parser-owned mutable state is
organized behind readable state bundles and explicit lifetime boundaries, so
future scope and text-identity work no longer depends on a giant flat
`Parser` object.

## Why This Idea Exists

The parser has accumulated several overlapping kinds of state on the main
`Parser` class:

- token/input ownership
- typedef/value/concept bindings
- record and enum definition registries
- namespace visibility state
- template scope bookkeeping
- pragma stacks
- tentative-parse rollback state
- parse diagnostics and debug context

Some of those areas already use stack-like push/pop rules, while others rely
on direct mutation plus ad-hoc rollback. That makes the class hard to read and
also makes later `TextId`-first cleanup harder than it should be, because
binding lifetime is not modeled in one obvious place.

The recent `parser_state.hpp` extraction gives a good seam, but the split is
not yet the real convergence point. The remaining work is to turn that seam
into a stable parser-state layout rather than a header relocation only.

## Main Objective

Make parser state readable and regrouped before deeper semantic changes.

The intended direction is:

- move parser state-bearing types into `parser_state.hpp`
- regroup parser-owned mutable state into named bundles with clear ownership
- separate semantic lexical scope from namespace, pragma, debug, and tentative
  rollback mechanisms
- prepare the parser for later shared-helper and `TextId` cleanup without
  forcing those migrations into the same runbook

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- parser implementation files that directly depend on parser-owned mutable
  state layout

## Candidate Directions

1. Create named parser-state bundles such as input state, binding state,
   record-definition state, template state, namespace state, diagnostic state,
   pragma state, and active contextual state.
2. Move state-supporting structs and snapshot/support types out of
   `parser.hpp` and into `parser_state.hpp` when they are not part of the true
   parser entry surface.
3. Reorder parser declarations so method families follow the implementation
   file split instead of being interleaved with state definitions.
4. Audit which parser mechanisms are real semantic scope push/pop systems and
   which are separate namespace/template/pragma/debug/tentative facilities.
5. Introduce a clearer semantic-scope seam only after the regrouped state
   layout makes the owned lifetime rules obvious.

## Constraints

- prioritize readability and ownership clarity before optimization
- avoid changing parser grammar behavior unless a structural move requires a
  small correctness-preserving fix
- do not turn this idea into a repo-wide string/container migration
- do not absorb sema, HIR, or backend restructuring here
- do not overfit the work to one failing testcase; this is a structural parser
  refactor route

## Validation

At minimum:

- `cmake --build build -j --target c4c_frontend c4cll`
- focused parser/frontend tests that cover tentative parsing and scope-heavy
  paths
- broader `ctest` coverage when the state regrouping reaches across multiple
  parser subsystems

## Non-Goals

- no immediate replacement of every parser string path with `TextId`
- no new global parser architecture outside the parser subsystem
- no backend work
- no promise that one runbook will finish every scope push/pop cleanup

## Suggested Execution Decomposition

1. Consolidate state-bearing structs into `parser_state.hpp` and keep
   `parser.hpp` focused on parser API plus method index.
2. Regroup parser member fields into explicit state bundles without changing
   behavior.
3. Classify current push/pop and rollback paths by kind:
   semantic lexical scope, namespace context, template scope, pragma stack,
   debug context, tentative rollback.
4. Use that classification to define the next narrower follow-on slice for
   semantic scope convergence and later `TextId` cleanup.
