# 130 AArch64 dispatch family post-contract layout audit

## Goal

Re-audit the AArch64 dispatch-family codegen files after the prepared producer,
edge-publication, current-block, calls, memory, wide-value, and i128-shift
contract work has closed.

This idea is analysis-only. It should not move implementation code directly.
Its job is to classify the remaining `dispatch*.cpp` files by durable owner
boundary and produce focused follow-up ideas only for concrete fold-back or
contract gaps.

## Why This Exists

Idea 115 intentionally did not create a mechanical fold-back idea for the
dispatch family because the live residue was still contract/shared-authority
oriented. Idea 116 then closed that contract residue: AArch64 dispatch now
consumes shared prepared facts for edge-copy materializable producer
classification, edge-publication source consistency, current-block join
instruction routing, and direct-global select-chain dependency lookup.

That changes the question. The next question is no longer "which dispatch
logic should move forward to BIR/prealloc?" first. It is:

- which `dispatch_*` files are still useful public hook boundaries;
- which files are now historical splits that should fold back into
  `dispatch.cpp` or a narrower reference-style owner;
- which remaining code is target-local emission glue that should stay in
  AArch64;
- whether any new shared-policy rediscovery remains despite idea 116.

Current dispatch-family files include:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

The reference layout under `ref/claudes-c-compiler/src/backend/arm/codegen`
does not have an equivalent pile of dispatch-specific translation units. If a
current file only exists because earlier work split out temporary helper
surface, it should be turned into a bounded follow-up fold-back idea. If a file
is an intentionally retained API/hook boundary, the closure note should say so
explicitly.

## Required Analysis

For each dispatch-family file, classify its remaining responsibilities as one
or more of:

- `keep-public-hook`: externally used entry point or stable orchestration hook
  that should remain a separate surface for now;
- `fold-back`: target-local helper surface that should move into
  `dispatch.cpp`, `memory.cpp`, `calls.cpp`, `comparison.cpp`, `variadic.cpp`,
  or another natural owner;
- `shared-contract-gap`: target-neutral producer, publication, edge-copy,
  current-block, or select-chain fact still rediscovered by AArch64 codegen;
- `target-local-emission`: AArch64 move/load/store/register hazard or final
  machine instruction emission;
- `local-organization-only`: private helper grouping that may be kept or folded
  only for readability.

The audit must explicitly inspect:

- public declarations in the matching `dispatch*.hpp` files;
- call sites of each dispatch-family public function;
- whether `dispatch_edge_copies.cpp` entry points retained by idea 81 are still
  externally used hooks;
- whether idea 116 left `dispatch_producers.cpp` as a real owner or a small
  consumer wrapper;
- whether `dispatch_publication.cpp` and
  `dispatch_value_materialization.cpp` still need separate translation units;
- whether `dispatch_lookup.cpp` is a useful query boundary or just a thin
  file-local helper split;
- build metadata impact for any proposed fold-back follow-up.

## Expected Output

The closure note must contain:

- a dispatch-family ownership table covering every `dispatch*.cpp` and
  `dispatch*.hpp` file;
- a call-site map for public dispatch-family declarations;
- explicit no-new-idea notes for files that should remain public hooks;
- follow-up ideas for each concrete fold-back or shared-contract gap, with
  bounded files, proof route, and reject signals;
- a note on whether the physical file count can now shrink without changing
  behavior.

If no follow-up is warranted, close with a clear explanation of why the current
dispatch-family split remains useful despite differing from the reference
layout.

## Reject Signals

- Moving code only to reduce line count or file count.
- Merging public hook surfaces while leaving scattered forward declarations,
  hidden cross-file dependencies, or unclear ownership.
- Reintroducing predecessor rescans, BIR-name matching, same-block named-case
  matching, or select-chain special cases.
- Moving AArch64 register hazard policy, instruction spelling, or final
  emission into shared BIR/prealloc code.
- Creating vague ideas such as "clean dispatch" without concrete files and a
  proof route.
- Weakening edge-copy, publication, current-block, select-chain, or backend
  expectations.

## Suggested Proof Route For Follow-Ups

This idea itself is analysis-only. Follow-up implementation ideas should choose
their narrow proof based on touched files. Likely candidates include:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`
- a broader `^backend_` run if a shared prepared query or dispatch contract is
  touched

## Closure Note

Closed after the analysis-only runbook completed the dispatch-family inventory,
public call-site map, ownership classification, retained-hook rationales, and
follow-up extraction. No implementation files, build metadata, or backend test
expectations were changed by this audit.

### Ownership Table

| File | Classification | Closure rationale |
| --- | --- | --- |
| `src/backend/mir/aarch64/codegen/dispatch.cpp` | `keep-public-hook`, `target-local-emission`, `local-organization-only` | Keep as the top-level prepared-block lowering owner. `make_block_lowering_context` and `dispatch_prepared_block` are called from `traversal.cpp`; the file also owns AArch64 instruction dispatch, branch-fusion hook wiring, before/after call and return publication sequencing, and target-local emission. |
| `src/backend/mir/aarch64/codegen/dispatch.hpp` | `keep-public-hook` | Keep as the public orchestration boundary for traversal. `InstructionDispatchResult` belongs to the `dispatch_prepared_block` return contract. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `target-local-emission`, `local-organization-only`, `fold-back` candidate, `shared-contract-gap` candidate | Owns AArch64 edge-copy emission, edge publication materialization, register hazard checks, and predecessor select parallel-copy lowering. Public entry points are dispatch-family-only or internal, so public header exposure can be narrowed. Fact lookup around edge producers/publication remains a shared-query candidate, but final instruction spelling stays target-local. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp` | `local-organization-only`, `fold-back` candidate | Exposes edge-copy helper types and functions used only by `dispatch_edge_copies.cpp` or nearby `dispatch.cpp` flow. Candidate for privatization or fold-back once behavior-preserving proof is attached. |
| `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp` | `keep-public-hook`, `fold-back` candidate, `shared-contract-gap` candidate | Retain the externally used lookup hooks `make_named_prepared_result_register` and `emitted_scalar_value_available`. The file is a thin query split and also exposes no-direct-caller helpers, so a follow-up can trim stale public surface or rehome lookup facts under a better owner. |
| `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp` | `keep-public-hook`, `fold-back` candidate, `shared-contract-gap` candidate | Header mixes real cross-file query hooks with no-direct-caller declarations: `is_scalar_call_argument_producer_opcode`, `find_same_block_scalar_producer`, and `has_same_block_load_local_producer`. Keep the real query boundary; follow-up should target only stale or rediscovery surface. |
| `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` | `keep-public-hook`, `local-organization-only`, `shared-contract-gap` candidate | Owns producer/source query hooks consumed by select/fp materialization, comparison hook tables, dispatch publication, ALU, and value-materialization hazards. Current-block routing and prepared fact reconstruction are candidates for shared prepared-query follow-up, not immediate fold-back. |
| `src/backend/mir/aarch64/codegen/dispatch_producers.hpp` | `keep-public-hook`, `local-organization-only`, `shared-contract-gap` candidate | Keep externally used producer contracts: select producer lookup, publication source producer hooks, producer instruction/index lookup, and publication register-read query. Dispatch-only routing helpers are local organization until a shared prepared-query contract replaces the underlying fact relationship. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `keep-public-hook`, `target-local-emission`, `shared-contract-gap` candidate | Keep separate: broad external users consume scalar/register view utilities, relocation operands, prepared value-home/current-block publication queries, and comparison branch-fusion hooks. A shared current-block/value-home fact query may be warranted, but AArch64 register/operand spelling remains target-local. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.hpp` | `keep-public-hook`, `shared-contract-gap` candidate | Keep public surface for scalar/register/publication utilities used across prepared value-home materialization, globals, select materialization, variadic, ALU, casts, memory, calls, comparison hooks, and dispatch-family code. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | `keep-public-hook`, `target-local-emission`, `shared-contract-gap` candidate | Keep separate: `emit_value_publication_to_register` is broadly consumed and the implementation owns target-local constants, loads, casts, select chains, binary values, scratch register handling, and hazard avoidance. Shared-query work should target fact lookup, not this emission hook. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp` | `keep-public-hook` | Keep as the public header for `emit_value_publication_to_register`, which is used by fp materialization, select materialization, comparison hooks, ALU, casts, memory, calls, edge copies, and publication code. |

### Public Call-Site Map

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
  `emitted_scalar_value_available` is used by calls. The other public
  declarations had no direct callers beyond declaration/definition.
- `dispatch_producers.hpp`: externally used hooks include
  `find_prepared_same_block_select_producer`,
  `prepared_publication_source_producer_for_value`,
  `prepared_source_producer_instruction`, `producer_instruction_index`, and
  `value_publication_may_read_register_index`. Current-block routing helpers
  are dispatch-family-only; `select_chain_contains_direct_global_load` had no
  direct callers beyond declaration/definition.
- `dispatch_publication.hpp`: scalar, register-view, relocation,
  prepared-home, current-block publication, and branch-fusion helpers have
  broad external users across prepared value-home materialization, fp
  materialization, globals, select materialization, variadic, ALU, casts,
  memory, calls, comparison hooks, and dispatch-family code.
- `dispatch_value_materialization.hpp`: `emit_value_publication_to_register`
  is a broad materialization hook used by fp materialization, select
  materialization, comparison hooks, ALU, casts, memory, calls, edge copies,
  publication, and recursive value materialization.

### Retained Public Hooks

- Keep `dispatch.hpp` public because traversal owns the outer walk and calls
  the dispatch orchestration hooks directly.
- Keep `dispatch_lookup.hpp` query hooks
  `make_named_prepared_result_register` and `emitted_scalar_value_available`
  because they are consumed by non-dispatch lowering files.
- Keep `dispatch_producers.hpp` producer hooks consumed by select/fp
  materialization, comparison hook tables, ALU, and value-materialization
  hazard checks.
- Keep `dispatch_publication.hpp` scalar/register/publication utility surface
  because it is a broad AArch64 codegen contract.
- Keep `dispatch_value_materialization.hpp` and
  `dispatch_value_materialization.cpp` because the single exported hook is
  widely consumed and the implementation is substantial target-local emission
  logic.
- Keep `dispatch_publication.cpp` separate on current evidence because it
  combines broad public hooks with AArch64 register/operand logic; do not
  create a file-count-only cleanup idea for it.

### Follow-Up Ideas Created

- `ideas/open/131_aarch64_dispatch_edge_copy_helper_surface_privatization.md`
- `ideas/open/132_aarch64_dispatch_lookup_thin_helper_surface_trim.md`
- `ideas/open/133_shared_prepared_fact_query_surface_extraction.md`
- `ideas/open/134_shared_select_chain_same_block_dependency_queries.md`
- `ideas/open/135_shared_current_block_entry_publication_query.md`

Physical file count can shrink without behavior changes only through a bounded
follow-up that actually narrows public surface or removes stale helper
exposure. The concrete file-count candidates are ideas 131 and 132. Ideas 133,
134, and 135 are shared-contract/query-shape work and should not be presented
as file-count reduction.

`dispatch_publication.cpp` and `dispatch_value_materialization.cpp` should
remain separate on current evidence. They have broad public hook surfaces and
substantial target-local AArch64 register, operand, hazard, and final emission
logic. The reference ARM backend having fewer dispatch-specific files is useful
layout evidence, not a mandate to collapse public AArch64 hook boundaries.

Close proof used matching backend logs:

`bash -o pipefail -c "cmake --preset default && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'" > test_before.log 2>&1`

and the same command to refresh `test_after.log`; `c4c-regression-guard`
accepted the lifecycle-only close in non-decreasing mode with 179/179 backend
tests passing before and after, no new failures, and no resolved failures.
