Status: Active
Source Idea Path: ideas/open/136_bir_prealloc_prepared_query_surface_simplification_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Shared Contract Quality

# Current Packet

## Just Finished

Completed `plan.md` Step 3: classified the prepared/prealloc query groups by
shared-contract quality and simplification value. Inspection used the Step 2
maps plus targeted `rg`/source reads in `prepared_lookups.*`,
`publication_plans.*`, `calls.hpp`, and `addressing.hpp`; AST-backed lookup was
not needed.

### Shared Contract Classification

| Domain | Classification | Evidence | Simplification value |
| --- | --- | --- | --- |
| Value-home and prepared storage lookup | good shared semantic fact, with a small wrong-owner subcase | `PreparedValueHomeLookups`, indexed value-home/id lookup, storage decoding, and block-entry publication availability expose target-neutral allocation/home facts. `find_frame_slot_by_id` and `find_stack_object_by_id` are good facts but live in `prepared_lookups.*` even though stack-layout/frame data is their natural owner. | Keep the value-home/storage contract. Moving only the frame/stack id helpers to a stack-layout/frame owner would help future x86/riscv discover stack facts without depending on the broad lookup facade; deleting the shared lookup layer would merely push AArch64 back to repeated home scans. No new idea for value-home/storage as a group; possible bounded owner-placement note for frame/stack id helpers if Step 4 wants one. |
| Current-block entry publication | good shared semantic fact | `find_prepared_current_block_entry_publication` returns destination value/home/publication availability. AArch64 `dispatch_publication.cpp` still owns register parsing, clobber checks, and emission policy. | Simplifying this would only reshuffle AArch64 wrapper code or reintroduce local publication scans. Future x86/riscv benefit from the same current-block publication fact before applying their own register/hazard policy. No new idea. |
| Edge-publication and move-bundle facts | mixed: good shared semantic fact plus suspicious AArch64-shaped facade subqueries | `PreparedEdgePublicationLookups`, unique edge publication lookup, move-bundle indexes, and publication/source validation are reusable control-flow/value-location facts. `prepare_current_block_join_parallel_copy_source_facts` and typed/aggregate stack-source helpers expose fields such as destination register name, source/destination register sharing, same-width i32 stack publication, and concrete emission-oriented routing; these are close to AArch64 edge-copy lowering shape. | Keep indexed edge publication and move-bundle facts because x86/riscv would also need edge source/home/move authority. A bounded follow-up could split target-neutral publication facts from lowering-routing conveniences so future backends can consume facts without inheriting AArch64 dispatch assumptions. This would help future x86/riscv if it removes register-name-shaped routing from the shared API; a broad move of all edge helpers would only reshuffle AArch64 code. |
| Same-block producer and materialization facts | good shared semantic fact | Same-block scalar/global-load producer, integer constant, fused compare operand, materialized condition, and load-local/store-source producer queries summarize BIR producer dependencies before target-specific materialization. AArch64 consumers use the result to choose local instruction forms and scratches. | Keep as shared. Future x86/riscv avoid rediscovering same-block producers, select roots, and memory producers. Splitting only because definitions span `prepared_lookups.cpp`, `select_chain_lookups.cpp`, and `publication_plans.cpp` is not useful without a public-owner cleanup. No new idea for semantics. |
| Select-chain dependency facts | too narrow or duplicated | The same select-chain-facing declarations are public in both `prepared_lookups.hpp` and `publication_plans.hpp`, while definitions live in `select_chain_lookups.cpp`. The facts themselves are reusable, but the public surface has duplicate ownership. | A bounded include/API-owner cleanup would help future x86/riscv by giving select-chain dependency queries one stable public owner. It should not change semantics or push callers back to source-producer scans. This is a real Step 4 candidate. |
| Call-plan argument, result, outgoing-stack, and boundary facts | good fact in the wrong owner file | Call plans, ABI argument/result bindings, outgoing stack areas, preservation, and boundary effects are semantic call-domain facts. The indexed caches in `PreparedCallPlanLookups` are useful, but their public owner is split between `prepared_lookups.*`, `calls.hpp`, and `call_plans.cpp`. | Moving or re-exporting call-plan indexes under a call-domain owner would help future x86/riscv understand ABI facts without treating `prepared_lookups.*` as a global dumping ground. The current data should not be deleted; AArch64-specific register views and instruction forms remain target-local. Potential Step 4 candidate if bounded to call-index ownership, not call lowering. |
| Memory-access and pointer/value-home facts | good fact in the wrong owner file | `PreparedMemoryAccessLookups`, result-name/result-id indexes, frame-address-offset queries, address materialization collection, and global-load access facts are semantic addressing facts. `addressing.hpp` already owns direct address/access structures and inline direct lookups; `publication_plans.*` owns store-source publication plans. | Moving memory/address indexes closer to `addressing.hpp` would help future x86/riscv by making addressing-domain facts discoverable without the broad prepared lookup facade. Store-source publication planning is reusable when it stays semantic; target-specific addressing modes remain local. Potential Step 4 candidate if limited to addressing ownership. |
| Preservation and runtime-helper facts | good fact in the wrong owner file | Prior preserved value lookups and stack-preserved call queries are call-preservation facts, not generic lookup facts. Runtime-helper and carrier finders already live mostly in runtime-helper modules and module-level prepared helper collections. | Keep the semantic facts, but do not create a broad runtime-helper simplification idea. Moving prior-preservation indexes toward call ownership could pair with the call-plan owner cleanup. Runtime-helper cleanup would mostly reshuffle AArch64 helper emission and is not justified here. No separate new idea. |
| Control-flow, join, compare, and parallel-copy facts | good shared semantic fact | `control_flow.hpp` and out-of-SSA data own branch targets, join transfers, compare/short-circuit contexts, and parallel-copy records; `prepared_lookups.*` consumes them for publication and move facts. AArch64 comparison/edge code uses these as semantic authority. | Keep under control-flow/out-of-SSA ownership. Future x86/riscv benefit from the same branch/join facts. Simplification outside the edge-publication facade would only move code around. No new idea. |

### No-New-Idea Notes

- No new idea for value-home/storage lookup as a whole: the contract is
  reusable, and simplification should not force target backends to scan homes
  or decode storage locally.
- No new idea for current-block entry publication: the shared query is semantic
  and target-local policy already stays in AArch64 dispatch code.
- No new idea for same-block producer/materialization semantics: the facts are
  target-neutral and reduce local rediscovery for any backend.
- No new idea for control-flow/join/compare facts: ownership is already clean
  enough, and the audit should not pull control-flow APIs into a facade cleanup.
- No separate runtime-helper idea from this audit: runtime-helper ownership is
  mostly already domain-local; preservation ownership should be considered only
  as part of call-plan lookup ownership.

### Candidate Follow-Up Shapes for Step 4

| Candidate | Why it is coherent | Reject signal |
| --- | --- | --- |
| Select-chain public-owner cleanup | Remove duplicate declarations between `prepared_lookups.hpp` and `publication_plans.hpp` and make `select_chain_lookups.cpp` queries have one public include boundary. | Reject if the slice changes select-chain semantics, rewrites AArch64 materialization, or makes callers manually rescan producers. |
| Call-plan lookup ownership cleanup | Move or re-export `PreparedCallPlanLookups` and preservation-related indexes through call-domain ownership while preserving `PreparedFunctionLookups` construction convenience. | Reject if it touches AArch64 call emission policy, register parsing, ABI expectation behavior, or call-boundary planning semantics. |
| Addressing lookup ownership cleanup | Move memory-access/address-materialization lookup caches and frame-address-offset queries toward the addressing/frame owner instead of the generic prepared lookup facade. | Reject if it changes address lowering, TLS/AArch64 relocation behavior, store-source publication semantics, or forces AArch64 memory code into local scans. |
| Edge-copy facade split | Separate target-neutral edge publication/source/home/move facts from current-block join parallel-copy routing and typed stack-source convenience fields that encode register-name/emission shape. | Reject if it moves AArch64 emission policy into shared code, deletes reusable edge publication facts, or introduces backend-specific named-case shortcuts. |

## Suggested Next

Execute `plan.md` Step 4: draft bounded follow-up ideas only for the real
opportunities above. The strongest candidates are select-chain public-owner
cleanup, call-plan lookup ownership cleanup, addressing lookup ownership
cleanup, and a narrow edge-copy facade split. Do not draft ideas for clean
semantic groups whose simplification would only move AArch64 code around.

## Watchouts

- Preserve cheap per-function aggregate construction (`PreparedFunctionLookups`)
  even if public ownership moves by domain.
- Do not mistake AArch64 call-site size for shared API leakage; most large
  consumers are target-local emission code reading valid semantic facts.
- Edge-copy cleanup is only valuable if it separates reusable publication facts
  from register-name/routing convenience. A broad edge rewrite would be route
  drift.
- No query group was classified as "no longer needed after later cleanup."
  Obsolete/deletion follow-ups are not justified by this packet.

## Proof

Analysis-only inspection packet. No build/test command was required or run, and
no `test_after.log` was created for this packet.
