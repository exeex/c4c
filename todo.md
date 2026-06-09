Status: Active
Source Idea Path: ideas/open/130_aarch64_dispatch_family_post_contract_layout_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Ownership And Contract Boundaries

# Current Packet

## Just Finished

Completed `plan.md` Step 3: Classify Ownership And Contract Boundaries.

Ownership table for every audited AArch64 dispatch-family file:

| File | Classification | Rationale |
| --- | --- | --- |
| `src/backend/mir/aarch64/codegen/dispatch.cpp` | `keep-public-hook`, `target-local-emission`, `local-organization-only` | Owns the top-level block lowering API (`make_block_lowering_context`, `dispatch_prepared_block`) called from `traversal.cpp`; also owns AArch64 instruction dispatch, branch-fusion hook wiring, before/after call and return publication sequencing, and per-block target emission. Local helpers such as diagnostics, BIR block lookup, and address-materialized lowering are internal organization and do not imply a new shared contract. |
| `src/backend/mir/aarch64/codegen/dispatch.hpp` | `keep-public-hook` | Header is the durable orchestration boundary exported to traversal: `make_block_lowering_context` and `dispatch_prepared_block` are external public hooks. `InstructionDispatchResult` has no independent external use, but it is part of the dispatch return contract and should remain with the top-level dispatch API. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | `target-local-emission`, `local-organization-only`, `fold-back` candidate, `shared-contract-gap` candidate | Owns AArch64-specific edge copy emission, register hazard checks, select-chain edge publication emission, and predecessor select parallel-copy lowering. Step 2 found its public entry points are called only by nearby dispatch-family code (`dispatch.cpp` and internal edge-copy recursion), so the separate public header surface is not externally justified. Repeated prepared edge-publication/source-producer lookup and select-chain hazard traversal mirror facts already in `prepare`/`prealloc`, making this a concrete shared-contract-gap candidate for Step 4, but the instruction spelling and scratch-register emission remain target-local. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp` | `local-organization-only`, `fold-back` candidate | Exposes `EdgeProducerContext`, edge publication hazard helpers, edge load/publication emitters, `should_emit_block_entry_edge_copy_move`, and `lower_predecessor_select_parallel_copy_sources`; Step 2 found these are dispatch-family-only or unused outside edge-copy implementation. Header can be considered for fold-back into a private dispatch-family boundary once Step 4 drafts a bounded follow-up. |
| `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp` | `keep-public-hook`, `fold-back` candidate, `shared-contract-gap` candidate | Small query unit that exposes externally used lookup hooks: `make_named_prepared_result_register` is used by globals, memory retargeting, comparison, ALU, memory, and calls; `emitted_scalar_value_available` is used by calls. The file is only 169 lines and the remaining declarations have no direct callers, so it is a thin helper split physically. However, its externally used register-availability queries justify retaining a public contract until Step 4 can decide whether the thin unused helpers should fold back or become shared prepared facts. |
| `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp` | `keep-public-hook`, `fold-back` candidate, `shared-contract-gap` candidate | Header exports real cross-file lookup hooks plus unused/no-direct-caller helpers (`is_scalar_call_argument_producer_opcode`, `find_same_block_scalar_producer`, `has_same_block_load_local_producer`). The useful boundary is a public query contract; the unused helper declarations and same-block producer rediscovery are concrete Step 4 candidates. |
| `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` | `keep-public-hook`, `local-organization-only`, `shared-contract-gap` candidate | Owns producer/source-query hooks used externally by select materialization, fp value materialization, comparison hook tables, dispatch publication, and ALU/value-materialization hazard checks. It also owns current-block join routing helpers called only by `dispatch.cpp` and local fallback construction of prepared value-home and edge-publication lookups. That fallback/fact reconstruction is the clearest shared-contract-gap candidate; public producer hooks should be retained unless Step 4 scopes a shared prepared-query extraction. |
| `src/backend/mir/aarch64/codegen/dispatch_producers.hpp` | `keep-public-hook`, `local-organization-only`, `shared-contract-gap` candidate | Header mixes externally used producer contracts (`find_prepared_same_block_select_producer`, `prepared_publication_source_producer_for_value`, `prepared_source_producer_instruction`, `producer_instruction_index`, `value_publication_may_read_register_index`) with dispatch-only current-block routing types/functions and no-direct-caller helpers. The external hooks remain justified; dispatch-only routing and local lookup reconstruction are candidates for bounded follow-up notes. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | `keep-public-hook`, `target-local-emission`, `shared-contract-gap` candidate | Still justifies a separate translation unit: Step 2 found broad external users for scalar/register view utilities, relocation operand building, prepared value-home/publication queries, current-block publication register lookup, and comparison branch-fusion hooks. The helper also contains target-local AArch64 operand/register spelling. The remaining potential gap is that current-block entry publication collection and value-home lookup are target-local wrappers around prepared facts; Step 4 should decide whether that becomes shared query surface, but not fold this file back wholesale. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.hpp` | `keep-public-hook`, `shared-contract-gap` candidate | Broad public contract used across prepared value-home materialization, globals, select materialization, variadic, ALU, casts, memory, calls, comparison hooks, and dispatch-family code. Retain as public hook surface; only the current-block publication/value-home query shape is a concrete shared-contract-gap candidate. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | `keep-public-hook`, `target-local-emission`, `shared-contract-gap` candidate | Single exported hook, `emit_value_publication_to_register`, is broadly used by fp value materialization, select materialization, comparison hooks, ALU, casts, memory, calls, edge copies, and dispatch publication. The implementation is target-local emission glue for constants, loads, casts, select chains, binary values, scratch registers, and hazard avoidance. It also re-queries prepared same-block scalar/select/constant facts and direct-global select-chain dependencies locally, which is a Step 4 shared-contract-gap candidate. Separate TU remains justified by the breadth and size of this emission hook. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp` | `keep-public-hook` | Header exports only the broad `emit_value_publication_to_register` materialization hook. Because the hook is externally consumed across many lowering files and branch-fusion tables, this is not a fold-back candidate on current evidence. |

Concrete Step 4 candidate set, without drafting new ideas yet:

- Fold-back candidates:
  - `dispatch_edge_copies.hpp` public declarations are dispatch-family-only and could become private to edge-copy/dispatch organization if a bounded follow-up keeps behavior and tests unchanged.
  - `dispatch_lookup.cpp`/`.hpp` are a thin query split; only the externally used lookup hooks are durable, while no-direct-caller helper declarations can be scoped for fold-back or removal after proof.
- Shared-contract-gap candidates:
  - Prepared source-producer/value-home/edge-publication fallback reconstruction appears in `dispatch_producers.cpp`, `dispatch_lookup.cpp`, `dispatch_edge_copies.cpp`, and `dispatch_value_materialization.cpp`; a future shared prepared-query contract should reject target-local rediscovery of the same facts.
  - Select-chain direct-global dependency and same-block scalar/constant materialization are queried locally by dispatch producers and value materialization; Step 4 should decide whether those facts belong in shared prepared/prealloc query helpers.
  - Current-block entry publication lookup in `dispatch_publication.cpp` is a target-local wrapper over prepared value-home/current-block publication facts; keep AArch64 register spelling local, but consider a shared query for the fact relationship.

Reference ARM layout evidence:

- `ref/claudes-c-compiler/src/backend/arm/codegen/mod.rs` has conventional codegen modules (`emit`, `alu`, `comparison`, `calls`, `memory`, `globals`, etc.) and no `dispatch`, `publication`, `producer`, or `edge` module.
- `ref/claudes-c-compiler/src/backend/arm/codegen/README.md` describes a monolithic `ArmCodegen`/stack-accumulator organization. This supports treating the reference as organization evidence only; it does not override AArch64's prepared-contract public hooks or target-local emission grouping.

## Suggested Next

Execute Step 4 from `plan.md`: draft bounded follow-up idea notes or explicit
no-new-idea rationales for the fold-back and shared-contract-gap candidates
identified above, while retaining justified public hooks.

## Watchouts

- Do not turn the reference ARM layout into a mandate to collapse AArch64 files;
  the current AArch64 public hooks have external consumers absent from the
  reference backend.
- Keep AArch64 register names, scratch selection, relocation operand spelling,
  and emitted instruction text target-local even if a shared fact-query helper is
  drafted.
- `dispatch_publication.cpp` and `dispatch_value_materialization.cpp` still
  justify separate translation units on current evidence; Step 4 should not
  propose file-count reduction there without a stronger contract reason.
- Generic helper names (`relocation_operand`, `scalar_fp_register_view`,
  `scalar_gp_register_view`, `immediate_integer_bits`) have same-name local
  helpers elsewhere; classify by the AArch64 dispatch-publication declarations,
  not by raw name matches alone.

## Proof

Analysis-only packet. Ran focused inventory, AST symbol inspection, header
inspection, shared-fact searches, and reference ARM layout inspection. Captured
the concise proof summary in `test_after.log`.

No build/test run required because no implementation code was edited.
