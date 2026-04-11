# Parser Symbol Tables And Speculative State Compaction

Status: Closed
Last Updated: 2026-04-10

## Goal

Shrink parser-side speculative parsing cost and parser-state management
complexity by tightening the ownership and representation of mutable symbol
tables used during tentative parsing.

## Why This Idea Exists

Recent parser work already showed that speculative parsing cost is sensitive to
how much parser state must be copied or restored.

Even after the tentative-guard tiering work, the parser still owns several
mutable lookup tables directly on the main parser object, including things like:

- typedef name sets
- typedef type maps
- variable type maps
- namespace-name indexes
- `using` alias maps

Those structures are reasonable, but they currently act both as semantic state
and as low-level storage detail. That makes future optimizations harder than
they need to be.

## Main Objective

Create a cleaner parser-local "state tables" layer so speculative parsing and
lookup code depend on stable operations rather than directly on a pile of STL
containers.

The intended direction is:

- isolate mutation-heavy parser state behind helper types
- make rollback-sensitive state easier to snapshot, journal, or compact later
- prepare for better-fit containers where they are justified

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- parser implementation files that participate in tentative parsing

## Candidate Directions

1. Group parser symbol/state tables into named structs instead of many
   top-level members on `Parser`.
2. Distinguish dense ID-indexed data from string-keyed lookup data.
3. Audit `std::set<std::string>` and `std::unordered_map<std::string, ...>`
   usage on speculative paths and mark which ones are correctness-critical
   versus convenience-only.
4. Introduce parser-local wrappers so future storage changes do not require
   touching every call site.
5. Leave room for a later journaled rollback model without forcing it in the
   same slice.

## Constraints

- preserve tentative parsing correctness first
- keep this work parser-local; do not absorb sema/HIR storage redesign here
- do not promise a full custom-container migration in one runbook
- avoid backend work entirely

## Validation

At minimum:

- existing parser regression suite
- rollback-sensitive parser tests
- parser-heavy STL or EASTL parse-only workloads when relevant to the chosen
  sub-slice

## Non-Goals

- no backend container migration
- no repo-wide custom `string` / `vector` rollout
- no parser grammar behavior change unless required to preserve correctness

## Completion Notes

Completed on 2026-04-10.

This runbook completed the planned typedef/value symbol-table compaction slice:

- grouped the parser-owned typedef/value tables under `Parser::ParserSymbolTables`
- migrated the targeted speculative and parser-type/declaration call sites onto
  parser-owned helpers and guards
- removed the flattened compatibility bridge members from `Parser`
- validated the refactor with targeted parser/frontend/HIR regressions plus
  repeated monotonic full-suite `ctest` runs through 3360/3360 passing

## Deferred Follow-On Surface

The broader parser-state compaction theme remains open only as future work
outside this runbook. The main deferred surfaces observed during execution are:

- namespace / using-directive visibility state such as `current_namespace_`,
  `namespace_contexts_`, `named_namespace_contexts_`,
  `using_value_aliases_`, and `using_namespace_contexts_`
- template-scope bookkeeping centered on `template_scope_stack_`
- record-member typedef caching in `struct_typedefs_`

Those areas were not required to finish the typedef/value symbol-table slice
and should be handled in a separate idea if they are still worth pursuing.
