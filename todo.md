# Rvalue Reference Completeness Todo

Status: Active
Source Idea: ideas/open/rvalue_reference_completeness_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Inventory current reference support
- Current slice: classify the repo's existing `T&&`, `auto&&`, and `auto&`
  coverage across parser, sema, HIR, and runtime; keep the latest EASTL tuple
  frontier in `/usr/include/c++/14/bits/ranges_util.h:740` as the active
  library-facing validation probe

## Todo

- [ ] Step 1: Inventory current reference support
- [ ] Step 2: Parser completeness for reference-shaped declarations
- [ ] Step 3: Semantic binding and overload rules
- [ ] Step 4: HIR value-category and forwarding behavior
- [ ] Step 5: Library-facing validation

## Completed

- [x] Switched the active plan from
  `ideas/open/eastl_container_bringup_plan.md` to
  `ideas/open/rvalue_reference_completeness_plan.md`
- [x] Preserved the EASTL source idea's latest Step 3 progress and resumable
  blocker notes before deactivating the old runbook
- [x] Expanded the new reference-semantics idea to explicitly include both
  `auto&&` and `auto&` under the same deduction/collapsing audit track

## Next Slice

- enumerate existing internal tests for:
  `T&&`, `const T&&`, `auto&&`, `auto&`, move ctor/assign, ref-qualified
  methods, forwarding helpers, and deduction guides
- map current implementation touchpoints in parser, sema, and HIR where
  reference/value-category behavior is hard-coded
- identify the highest-value missing regression cluster, likely starting with
  `auto&&` / `auto&` deduction and block-scope declaration handling
- preserve the current EASTL tuple probe as the real-header check after each
  parser-side slice

## Blockers

- the latest library-facing tuple probe still reaches a later generic parser
  frontier around `/usr/include/c++/14/bits/ranges_util.h:740`
- current HIR logic still appears to rely heavily on lvalue/non-lvalue checks,
  which may hide xvalue-specific bugs
- `auto&&` parses as `TB_AUTO + is_rvalue_ref`, but full deduction and
  forwarding-reference semantics are not yet clearly implemented end-to-end
