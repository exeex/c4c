Status: Active
Source Idea Path: ideas/open/130_aarch64_dispatch_family_post_contract_layout_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Finalize Audit Closure Material
Runbook Completion: Complete for supervisor/plan-owner review

# Current Packet

## Just Finished

Completed `plan.md` Step 5: Finalize Audit Closure Material.

This analysis-only packet assembled the closure material from Steps 1-4 into
canonical `todo.md`. It did not edit implementation files, build metadata,
`plan.md`, the source idea, or create `ideas/open/*.md` follow-up files.

### Dispatch-Family Ownership Table

| File | Classification | Closure rationale |
| --- | --- | --- |
| `src/backend/mir/aarch64/codegen/dispatch.cpp` | `keep-public-hook`, `target-local-emission`, `local-organization-only` | Keep as the top-level prepared-block lowering owner. `make_block_lowering_context` and `dispatch_prepared_block` are called from `traversal.cpp`; the file also owns AArch64 instruction dispatch, branch-fusion hook wiring, before/after call and return publication sequencing, and target-local emission. |
| `src/backend/mir/aarch64/codegen/dispatch.hpp` | `keep-public-hook` | Keep as the public orchestration boundary for traversal. `InstructionDispatchResult` has no independent external use, but belongs to the `dispatch_prepared_block` return contract. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `target-local-emission`, `local-organization-only`, `fold-back` candidate, `shared-contract-gap` candidate | Owns AArch64 edge-copy emission, edge publication materialization, register hazard checks, and predecessor select parallel-copy lowering. Public entry points are dispatch-family-only or internal, so public header exposure can be narrowed. Fact lookup around edge producers/publication remains a shared-query candidate, but final instruction spelling stays target-local. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp` | `local-organization-only`, `fold-back` candidate | Exposes edge-copy helper types and functions that Step 2 found used only by `dispatch_edge_copies.cpp` or nearby `dispatch.cpp` flow. Candidate for privatization or fold-back once behavior-preserving proof is attached. |
| `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp` | `keep-public-hook`, `fold-back` candidate, `shared-contract-gap` candidate | Retain the externally used lookup hooks `make_named_prepared_result_register` and `emitted_scalar_value_available`. The file is a thin query split and also exposes no-direct-caller helpers, so a follow-up can trim stale public surface or rehome lookup facts under a better owner. |
| `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp` | `keep-public-hook`, `fold-back` candidate, `shared-contract-gap` candidate | Header mixes real cross-file query hooks with no-direct-caller declarations: `is_scalar_call_argument_producer_opcode`, `find_same_block_scalar_producer`, and `has_same_block_load_local_producer`. Keep the real query boundary; draft follow-up should target only stale or rediscovery surface. |
| `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` | `keep-public-hook`, `local-organization-only`, `shared-contract-gap` candidate | Owns producer/source query hooks consumed by select/fp materialization, comparison hook tables, dispatch publication, ALU, and value-materialization hazards. Current-block routing and prepared fact reconstruction are candidates for shared prepared-query follow-up, not immediate fold-back. |
| `src/backend/mir/aarch64/codegen/dispatch_producers.hpp` | `keep-public-hook`, `local-organization-only`, `shared-contract-gap` candidate | Keep externally used producer contracts: select producer lookup, publication source producer hooks, producer instruction/index lookup, and publication register-read query. Dispatch-only routing helpers are local organization until a shared prepared-query contract replaces the underlying fact relationship. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `keep-public-hook`, `target-local-emission`, `shared-contract-gap` candidate | Keep separate: broad external users consume scalar/register view utilities, relocation operands, prepared value-home/current-block publication queries, and comparison branch-fusion hooks. A shared current-block/value-home fact query may be warranted, but AArch64 register/operand spelling remains target-local. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.hpp` | `keep-public-hook`, `shared-contract-gap` candidate | Keep public surface for scalar/register/publication utilities used across prepared value-home materialization, globals, select materialization, variadic, ALU, casts, memory, calls, comparison hooks, and dispatch-family code. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | `keep-public-hook`, `target-local-emission`, `shared-contract-gap` candidate | Keep separate: `emit_value_publication_to_register` is broadly consumed and the implementation owns target-local constants, loads, casts, select chains, binary values, scratch register handling, and hazard avoidance. Shared-query work should target fact lookup, not this emission hook. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp` | `keep-public-hook` | Keep as the public header for `emit_value_publication_to_register`, which is used by fp materialization, select materialization, comparison hooks, ALU, casts, memory, calls, edge copies, and publication code. |

### Public Call-Site Map Summary

- `dispatch.hpp`: `make_block_lowering_context` and
  `dispatch_prepared_block` are external orchestration hooks called by
  `traversal.cpp`; `InstructionDispatchResult` is part of that API contract.
- `dispatch_edge_copies.hpp`: `EdgeProducerContext`,
  `edge_value_publication_may_read_register_index`,
  `emit_edge_load_local_to_register`,
  `emit_edge_value_publication_to_register`,
  `should_emit_block_entry_edge_copy_move`, and
  `lower_predecessor_select_parallel_copy_sources` are dispatch-family-only or
  internal to edge-copy flow.
- `dispatch_lookup.hpp`: `make_named_prepared_result_register` is used by
  globals, memory retargeting, comparison, ALU, memory, and calls;
  `emitted_scalar_value_available` is used by calls. The other three public
  declarations had no direct callers beyond declaration/definition.
- `dispatch_producers.hpp`: externally used hooks include
  `find_prepared_same_block_select_producer`,
  `prepared_publication_source_producer_for_value`,
  `prepared_source_producer_instruction`, `producer_instruction_index`, and
  `value_publication_may_read_register_index`. Current-block routing helpers
  are dispatch-family-only; `select_chain_contains_direct_global_load` had no
  direct callers beyond declaration/definition.
- `dispatch_publication.hpp`: scalar, register-view, relocation, prepared-home,
  current-block publication, and branch-fusion helpers have broad external
  users across prepared value-home materialization, fp materialization,
  globals, select materialization, variadic, ALU, casts, memory, calls,
  comparison hooks, and dispatch-family code.
- `dispatch_value_materialization.hpp`: `emit_value_publication_to_register`
  is a broad materialization hook used by fp materialization, select
  materialization, comparison hooks, ALU, casts, memory, calls, edge copies,
  publication, and recursive value materialization.

### No-New-Idea Rationales For Retained Public Hooks

- Keep `dispatch.hpp` public because traversal owns the outer walk and calls the
  dispatch orchestration hooks directly. No new idea is warranted unless
  traversal ownership changes.
- Keep `dispatch_lookup.hpp` query hooks
  `make_named_prepared_result_register` and `emitted_scalar_value_available`
  because they are consumed by non-dispatch lowering files. Any follow-up should
  trim stale/no-direct-caller helpers without deleting the real query contract.
- Keep `dispatch_producers.hpp` producer hooks consumed by select/fp
  materialization, comparison hook tables, ALU, and value-materialization hazard
  checks. Current-block routing names alone do not justify a standalone cleanup
  idea; they belong to a shared-query follow-up if one is opened.
- Keep `dispatch_publication.hpp` scalar/register/publication utility surface
  because it is a broad AArch64 codegen contract. Draft shared-query work should
  target only target-neutral current-block/value-home facts.
- Keep `dispatch_value_materialization.hpp` and
  `dispatch_value_materialization.cpp` because the single exported hook is
  widely consumed and the implementation is substantial target-local emission
  logic.
- Keep `dispatch_publication.cpp` separate on current evidence because it
  combines broad public hooks with AArch64 register/operand logic; do not create
  a file-count-only cleanup idea for it.

### Bounded Follow-Up Draft List

#### Draft A: Privatize Dispatch Edge-Copy Helper Surface

- Finding type: `fold-back`.
- Candidate files: `dispatch_edge_copies.hpp`, `dispatch_edge_copies.cpp`,
  `dispatch.cpp`, and `src/backend/CMakeLists.txt` only if a translation unit
  is removed.
- Boundary issue: edge-copy helper types/functions are public despite only
  dispatch-family/internal use.
- Expected behavior preservation: edge-copy instruction order, predecessor
  select parallel-copy behavior, and block-entry publication behavior must not
  change.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`.
- Reject signals: file-count-only movement, hidden cross-file dependencies,
  changed edge/publication ordering, or weakened expectations.

#### Draft B: Trim Or Rehome Thin Dispatch Lookup Helpers

- Finding type: `fold-back` plus possible `shared-contract-gap`.
- Candidate files: `dispatch_lookup.hpp`, `dispatch_lookup.cpp`, natural
  callers such as `calls.cpp`, `globals.cpp`, `memory.cpp`,
  `memory_store_retargeting.cpp`, `comparison.cpp`, `alu.cpp`, and
  `src/backend/CMakeLists.txt` only if a translation unit is removed.
- Boundary issue: the two real public lookup hooks should remain unless a
  better query owner replaces them; the no-direct-caller declarations are stale
  public surface or local rediscovery candidates.
- Expected behavior preservation: prepared-result register selection and
  call-lowering availability semantics must remain unchanged.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`.
- Reject signals: deleting externally used lookup hooks, BIR-name rescans, or
  treating small file size as sufficient justification.

#### Draft C: Extract Shared Prepared Fact Query Surface

- Finding type: `shared-contract-gap`.
- Candidate files: `dispatch_producers.cpp`, `dispatch_lookup.cpp`,
  `dispatch_edge_copies.cpp`, `dispatch_publication.cpp`,
  `dispatch_value_materialization.cpp`, plus the selected shared
  `src/backend/mir/prealloc/` or `src/backend/mir/prepare*` query owner.
- Missing shared fact: prepared value-home, edge-publication source-producer,
  current-block entry publication, and same-block materialization relationships
  are still stitched together locally in several AArch64 dispatch-family files.
- Expected behavior preservation: register names, scratch choices, hazard
  policy, relocation operands, and final instruction spelling stay AArch64
  local.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Reject signals: moving target-local emission into shared code, reintroducing
  predecessor rescans or BIR-name matching, or duplicating the current wrapper
  under a generic name.

#### Draft D: Share Select-Chain And Same-Block Dependency Queries

- Finding type: `shared-contract-gap`.
- Candidate files: `dispatch_producers.hpp`, `dispatch_producers.cpp`,
  `dispatch_value_materialization.cpp`, and the selected shared MIR/prealloc
  query owner.
- Missing shared fact: select-chain direct-global dependency and same-block
  scalar/constant materialization checks are still queried locally around
  producer and value-publication materialization.
- Expected behavior preservation: AArch64 materialization emission and
  register-read hazard handling remain local.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Reject signals: named-case select-chain shortcuts, direct-global special cases
  that do not generalize to the prepared fact model, or proof of only one
  current failing case.

#### Draft E: Clarify Current-Block Entry Publication Fact Query

- Finding type: `shared-contract-gap`.
- Candidate files: `dispatch_publication.hpp`, `dispatch_publication.cpp`,
  `dispatch_producers.cpp`, `dispatch_value_materialization.cpp`, and selected
  shared prepared/prealloc query files.
- Missing shared fact: current-block entry publication collection and register
  lookup are AArch64 wrappers over prepared current-block/value-home facts.
- Expected behavior preservation: publication register recording,
  branch-fusion hooks, and AArch64 operand/register spelling must not change.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Reject signals: broad publication rewrites, moving register-view selection
  out of AArch64, or proposing a generic query that does not replace an actual
  target-local rediscovery site.

### File-Count Shrink Conclusion

Physical file count can shrink without behavior changes only through a bounded
follow-up that actually narrows public surface or removes stale helper exposure.
The concrete candidates are Draft A (`dispatch_edge_copies` public helper
surface) and Draft B (`dispatch_lookup` thin/stale helper surface). Drafts C,
D, and E are shared-contract/query-shape work and should not be presented as
file-count reduction.

`dispatch_publication.cpp` and `dispatch_value_materialization.cpp` should
remain separate on current evidence. They have broad public hook surfaces and
substantial target-local AArch64 register, operand, hazard, and final emission
logic. The reference ARM backend having fewer dispatch-specific files is useful
layout evidence, not a mandate to collapse public AArch64 hook boundaries.

## Suggested Next

Supervisor should review this completed audit artifact and route to
plan-owner for lifecycle decision: close the active source idea if these
materials satisfy the audit, or convert selected drafts into separate
`ideas/open/*.md` follow-up initiatives. No implementation packet is pending
inside this runbook.

## Watchouts

- This packet intentionally did not create new `ideas/open/*.md` files; the
  draft list is closure material for supervisor/plan-owner review.
- Do not treat Draft C, D, or E as permission to move AArch64 register hazard
  policy, instruction spelling, scratch choices, relocation operands, or final
  emission into shared code.
- Any follow-up that touches shared prepared/prealloc query code needs broader
  `^backend_` proof. Fold-back-only AArch64 surface work can use
  `^backend_aarch64_` after a normal build.
- No test expectations were weakened and no implementation code was edited in
  this audit runbook.

## Proof

Analysis-only packet. Re-read the committed Step 1 inventory, Step 2 call-site
map, Step 3 ownership table, Step 4 follow-up/no-new-idea notes, `plan.md`, and
the source idea. Used focused `sed`/`git show` inspection only; no clang-tools
or build/test run was needed because no implementation files were edited.

Captured the concise proof summary in `test_after.log`.
