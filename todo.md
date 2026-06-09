Status: Active
Source Idea Path: ideas/open/136_bir_prealloc_prepared_query_surface_simplification_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Assemble Closure Recommendation

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by assembling the closure recommendation package
requested by the source idea. This packet is based on the committed Step 1-4
audit notes plus the bounded follow-up ideas now present under
`ideas/open/137-140`.

### Major Prepared-Query Table

| Domain / query group | Current public surface | Classification | Closure recommendation |
| --- | --- | --- | --- |
| Value-home and prepared storage lookup | `PreparedValueHomeLookups`, indexed value-home/id lookup, `find_prepared_value_home_for_bir_value`, storage decoding, block-entry publication availability, frame/stack id helpers | Good shared semantic fact, with a small wrong-owner subcase for frame/stack id helpers | Keep the shared value-home/storage contract. Do not create a broad follow-up. Frame/stack id helper placement may be handled opportunistically by future addressing/frame ownership cleanup, but it is not enough for a separate idea from this audit. |
| Current-block entry publication | `PreparedCurrentBlockEntryPublicationQueryInputs`, `find_prepared_current_block_entry_publication`, `collect_prepared_block_entry_publications`, availability helpers | Good shared semantic fact | Keep as shared. AArch64 register-view parsing, clobber policy, and emission remain target-local; no new idea. |
| Edge-publication and move-bundle facts | `PreparedEdgePublicationLookups`, edge publication keys, unique edge-publication lookup, edge-copy source facts, `PreparedMoveBundleLookups`, before/after call and return move queries | Mixed: reusable edge/move facts plus suspicious target-facing routing conveniences | Preserve reusable edge publication/source/home/move facts. Create a bounded follow-up to split current-block join parallel-copy and typed stack-source routing conveniences from target-neutral edge facts: `ideas/open/140_edge_copy_facade_split.md`. |
| Same-block producer and materialization facts | Same-block scalar/global-load producer, integer constant, fused compare operand, materialized condition, load-local/store-source producer queries | Good shared semantic fact | Keep as shared. These facts prevent backend rediscovery of BIR producers while leaving materialization order, scratch choice, and instruction selection target-local; no new idea. |
| Select-chain dependency facts | `PreparedSelectChainDependencyQuery`, direct-global select-chain dependency, source-producer, store-source dependency, scalar materialization, call-argument direct-global projections | Too narrow / duplicated public ownership | Keep the facts, but give them one stable public owner instead of split exposure through `prepared_lookups.hpp` and `publication_plans.hpp`: `ideas/open/137_select_chain_public_owner_cleanup.md`. |
| Call-plan argument/result/outgoing-stack/boundary facts | `PreparedCallPlanLookups`, indexed call/immediate/result/outgoing-stack lookups, prior-preservation indexes, republication effects, call-boundary effects, publication routing | Good shared call facts in the wrong public owner | Keep cached call facts and cheap `PreparedFunctionLookups` aggregation, but move or re-export lookup ownership through the call domain: `ideas/open/138_call_plan_lookup_ownership_cleanup.md`. |
| Memory-access, address-materialization, frame-address-offset, and global-load facts | `PreparedAddressMaterializationLookups`, `PreparedMemoryAccessLookups`, indexed memory/access queries, frame-address-offset lookup, address materialization collection, global-load access queries | Good addressing facts in the wrong public owner | Keep cached addressing facts, but move ownership toward addressing/frame modules instead of the generic lookup facade: `ideas/open/139_addressing_lookup_ownership_cleanup.md`. |
| Preservation and runtime-helper facts | Prior-preserved value queries, stack-preserved values for calls, runtime-helper and carrier availability/finders | Mostly good call/runtime facts, with preservation folded into call-owner mismatch | Do not create a separate runtime-helper idea. Preservation lookup ownership is covered by the call-plan cleanup; runtime-helper ownership is already mostly domain-local. |
| Control-flow, join, compare, and parallel-copy facts | Branch target, join transfer, compare/short-circuit, out-of-SSA parallel-copy, move-bundle authority | Good shared semantic fact under existing control-flow/out-of-SSA ownership | Keep under control-flow/out-of-SSA ownership. Do not pull this whole area into a prepared-lookup cleanup. |

### Domain Ownership Map

| Domain | Data owner | Current public query/API owner | Recommended owner direction |
| --- | --- | --- | --- |
| Value homes / storage | `value_locations.hpp`, `decoded_home_storage.*`, stack-layout data in `PreparedBirModule` | `value_locations.hpp`, `prepared_lookups.*`, `decoded_home_storage.*` | Keep value-home/storage queries shared. Avoid deleting indexes; consider frame/stack id helper placement only as part of a future addressing/frame owner cleanup. |
| Current-block entry publication | `PreparedValueLocationFunction` / `value_locations.hpp` | `prepared_lookups.*` plus collection helpers in `value_locations.hpp` | Keep current ownership acceptable because the shared query is semantic and AArch64 policy is outside it. |
| Edge publication / move bundles | `value_locations.hpp`, `control_flow.hpp`, `out_of_ssa.cpp` | Broadly `prepared_lookups.*` with adjacent control-flow helpers | Split fact ownership from routing conveniences. Reusable edge/move facts can remain shared; current-block join routing surfaces need clearer target-facing boundaries. |
| Same-block producer / materialization | Prepared control-flow, value-location, addressing, and BIR position data; select-chain definitions in `select_chain_lookups.cpp` | `prepared_lookups.hpp`, `publication_plans.hpp/.cpp`, `select_chain_lookups.cpp` | Keep same-block semantic facts. Only select-chain public ownership needs cleanup. |
| Select-chain dependencies | Source-producer traversal, call/publication planning, select-chain lookup implementation | Declarations split across `prepared_lookups.hpp` and `publication_plans.hpp`; definitions in `select_chain_lookups.cpp`; call projections in `calls.hpp` | Give select-chain facts one public include boundary while preserving domain projections. |
| Call plans / preservation / call-boundary effects | `call_plans.cpp`, `calls.hpp` | Split between `prepared_lookups.*`, `calls.hpp`, and `call_plans.cpp` | Move or re-export indexed call lookups through call-domain ownership while retaining aggregate construction. |
| Addressing / memory access / frame offsets | `addressing.hpp`, stack-layout coordinator/data, `publication_plans.*` for store-source planning | Direct facts in `addressing.hpp`; caches and frame-offset/global-load queries in `prepared_lookups.*`; store-source planners in `publication_plans.*` | Move memory/address lookup ownership toward addressing/frame owner modules. |
| Runtime helpers | `runtime_helpers.hpp`, `f128_runtime_helpers.*`, `i128_runtime_helpers.*`, `module.hpp` | Mostly runtime/module owners; preservation-adjacent helpers in `prepared_lookups.*` and `calls.hpp` | Leave runtime-helper ownership alone; handle prior-preservation indexes with call ownership. |
| Control-flow / joins / compares | `control_flow.hpp`, `out_of_ssa.cpp` | `control_flow.hpp` and adjacent prepared helpers | Leave as-is except where edge-copy facade cleanup needs to clarify boundaries. |

### AArch64 Consumer Map

| Domain | AArch64 consumers | Consumer conclusion |
| --- | --- | --- |
| Value-home / storage | `traversal.cpp`, `dispatch_publication.cpp`, `comparison.cpp`, `select_materialization.cpp`, `dispatch_lookup.cpp`, `alu.cpp`, `globals.cpp`, `returns.cpp`, `atomics.cpp`, `cast_ops.cpp`, `instruction.cpp`, `memory.cpp`, `calls.cpp`, `operands.cpp`, `frame_slot_address.cpp`, `memory_store_retargeting.cpp` | Consumers read semantic home/storage facts, then apply AArch64 register spelling, operand formation, and instruction selection locally. Deleting shared lookup caches would force local rediscovery. |
| Current-block entry publication | `dispatch_publication.cpp`, `dispatch_producers.cpp`, `dispatch_value_materialization.cpp`, `comparison.cpp`, `dispatch.cpp`, `comparison.hpp` | Shared query is clean; wrappers apply AArch64 clobber/register-view policy. |
| Edge publication / move bundles | `dispatch_producers.cpp`, `dispatch_edge_copies.cpp`, `cast_ops.cpp`, `calls.cpp` | Mixed consumer shape. Edge facts are reusable; join parallel-copy routing and typed stack-source helpers are close to AArch64 lowering shape and justify follow-up 140. |
| Same-block producer / materialization | `dispatch_producers.cpp`, `fp_value_materialization.cpp`, `dispatch_value_materialization.cpp`, `comparison.cpp`, `alu.cpp`, `memory.cpp`, `calls.cpp` | Consumers use semantic producer/dependency facts before local materialization, scratch, branch-fusion, and emission decisions. |
| Select-chain dependencies | `dispatch_producers.cpp`, `calls.cpp`, `dispatch_value_materialization.cpp`, `alu.cpp`, `memory.cpp` | Facts are semantic, but split public declarations make ownership unclear for future backends; follow-up 137 should fix the include/API owner only. |
| Call plans / call boundary / preservation | Dominantly `calls.cpp`; also `select_materialization.cpp`, `alu.cpp`, `memory.cpp`, `traversal.cpp`, `dispatch.cpp`, `globals.cpp` | Consumers are large because call lowering is target-local, not because the facts are invalid. Ownership should move toward call domain without changing ABI/emission behavior; follow-up 138. |
| Addressing / memory access | Dominantly `memory.cpp`; also `globals.cpp`, `frame_slot_address.cpp`, `comparison.cpp`, `alu.cpp`, `dispatch_value_materialization.cpp`, `memory_store_retargeting.cpp`, `calls.cpp`, AArch64 module context | Consumers use reusable address/materialization/global-load/frame facts and keep addressing modes, relocations, TLS behavior, and register materialization target-local; follow-up 139 should be ownership-focused. |
| Preservation / runtime helpers | `calls.cpp`, `i128_ops.cpp`, `f128.cpp`, `comparison.cpp`, `memory.cpp` | Runtime helper lookups are already domain-oriented. Prior-preservation lookups should move with call ownership, not create a standalone runtime cleanup. |
| Control-flow / joins / compares | `comparison.cpp`, `traversal.cpp`, indirect users through edge/current-block code | Clean shared control-flow authority. No broad follow-up needed. |

### Explicit No-New-Idea Decisions

- No new idea for value-home/storage lookup as a whole. The shared contract is
  reusable and prevents AArch64 or future backends from rescanning prepared
  homes or decoding storage locally.
- No standalone idea for frame/stack id helpers. Their owner placement is a
  small subcase and can be handled, if needed, inside addressing/frame owner
  cleanup.
- No new idea for current-block entry publication. The query is semantic and
  target-local policy already stays in AArch64 dispatch code.
- No new idea for same-block producer/materialization semantics. The facts are
  target-neutral and useful across backends.
- No new idea for broad control-flow/join/compare cleanup. Ownership is already
  coherent enough and should not be folded into a prepared-lookup facade split.
- No separate runtime-helper cleanup idea. Runtime-helper ownership is mostly
  already local to runtime/module domains, and prior-preservation ownership is
  covered by the call-plan follow-up.
- No obsolete/deletion idea. The audit found no query group that is no longer
  needed after later cleanup.

### Follow-Up Recommendation Summary

| Follow-up idea | Recommendation | Scope boundary |
| --- | --- | --- |
| `ideas/open/137_select_chain_public_owner_cleanup.md` | Keep as a future bounded implementation idea. | One public owner/include boundary for select-chain dependency facts; no semantic changes or local producer rescans. |
| `ideas/open/138_call_plan_lookup_ownership_cleanup.md` | Keep as a future bounded implementation idea. | Call-domain ownership for `PreparedCallPlanLookups`, outgoing-stack, prior-preservation, republication, and call-boundary lookups while preserving cheap aggregate construction. |
| `ideas/open/139_addressing_lookup_ownership_cleanup.md` | Keep as a future bounded implementation idea. | Addressing/frame-domain ownership for memory-access, address-materialization, frame-address-offset, and global-load lookup caches; no addressing-mode or stack-layout semantic changes. |
| `ideas/open/140_edge_copy_facade_split.md` | Keep as a future bounded implementation idea. | Separate reusable edge publication/source/home/move facts from current-block join parallel-copy and typed stack-source routing conveniences. |

### `prepared_lookups.*` Closure Recommendation

`prepared_lookups.*` should not stay as one ever-growing facade, and it should
not be split mechanically by line count. The recommended direction is to shrink
the facade by moving or re-exporting query ownership to existing domain owners:

- select-chain queries should have a single select-chain/publication-domain
  owner instead of duplicate public exposure;
- call-plan and preservation lookups should be owned at the call-domain
  boundary while remaining cheap to aggregate through `PreparedFunctionLookups`;
- memory/access/address-materialization/frame-offset lookups should move toward
  addressing/frame ownership;
- edge-copy surfaces should split reusable prepared edge facts from
  target-facing join routing conveniences.

The remaining shared lookup layer is justified when it expresses reusable
backend contracts and cached per-function lookup state. It should continue to
protect AArch64 and future x86/riscv backends from local BIR scans, predecessor
rescans, name matching, or testcase-shaped rediscovery. The next lifecycle
action can close this analysis plan with four bounded follow-up ideas and no
implementation changes under the active audit.

## Suggested Next

Plan-owner close review for
`ideas/open/136_bir_prealloc_prepared_query_surface_simplification_audit.md`.
The active runbook appears exhausted, and the closure package above contains
the requested prepared-query table, ownership map, AArch64 consumer map,
explicit no-new-idea decisions, follow-up idea summary, and
`prepared_lookups.*` recommendation.

## Watchouts

- Treat ideas 137-140 as future lifecycle candidates, not active work under
  this analysis plan.
- Reject any future simplification that deletes semantic prepared facts and
  sends backends back to local scans, predecessor rediscovery, name matching,
  expectation downgrades, or named-case shortcuts.
- Preserve cheap per-function aggregate lookup construction even when public
  ownership moves by domain.

## Proof

Analysis-only inspection packet. No build/test command was required or run, and
no `test_after.log` was created for this packet.
