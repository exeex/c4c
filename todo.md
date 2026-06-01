Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map Prepared Call And Publication Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 1 mapping for prepared call/publication authority.
No implementation files were edited.

| Local decision | Current AArch64 site | Existing prepared fact/query | Authority status |
| --- | --- | --- | --- |
| call-plan lookup and call spelling inputs | `calls.cpp:65`, `calls.cpp:93`, `calls.cpp:5463` | `PreparedCallPlan` via `find_indexed_prepared_call_plan` (`prepared_lookups.hpp:746`) | Present. AArch64 already requires the prepared plan before normal call lowering; local residue should stay limited to direct/indirect spelling and AArch64 register conversion. |
| call argument/result binding | `calls.cpp:2369`, `calls.cpp:3835`, `calls.cpp:5123` | `PreparedCallArgumentPlan`, `PreparedCallResultPlan`, `PreparedAbiBinding`, and `classify_prepared_call_boundary_move` (`calls.hpp:128`, `calls.hpp:169`, `calls.hpp:338`, `call_plans.cpp:2575`) | Present but still re-consumed through repeated target-local classification and fallback scans. First migration should consume the prepared classification/effect endpoint as the authority at the AArch64 lowering boundary. |
| call-boundary move bundle selection | `calls.cpp:4463`, `calls.cpp:4544`, `calls.cpp:4605`, `calls.cpp:4661`, `calls.cpp:4789` | `PreparedMoveBundle` via `find_indexed_prepared_move_bundle` (`prepared_lookups.hpp:811`, `prepared_lookups.cpp:2414`); `PreparedCallBoundaryEffectPlan` via `plan_prepared_call_boundary_effects` (`calls.hpp:372`, `call_plans.cpp:2809`) | Present. AArch64 still iterates raw bundle moves and rebuilds local `CallBoundaryMoveInstructionRecord` eligibility; prepared effect plans are the narrow shared authority to consume next. |
| after-call result lane binding | `calls.cpp:6278` | `PreparedAfterCallResultLaneBinding` (`prepared_lookups.hpp:71`) via `find_indexed_prepared_after_call_result_lane_binding` (`prepared_lookups.hpp:825`, `prepared_lookups.cpp:2452`) | Present and already consumed for scalar-state recording. Residue is AArch64 register conversion/view selection, which should remain local. |
| edge publication | `dispatch_publication.cpp:514`, `dispatch_publication.cpp:629` | `PreparedEdgePublication`, `PreparedEdgePublicationLookups`, and `PreparedEdgePublicationSourceProducer` (`prepared_lookups.hpp:364`, `prepared_lookups.hpp:438`, `prepared_lookups.hpp:535`) | Present. Some AArch64 paths use producer lookups; publication ordering remains sensitive and should not be invented locally. |
| edge-copy source | `dispatch_edge_copies.cpp:1252`, `dispatch_edge_copies.cpp:1290` plus publication helpers in `dispatch_publication.cpp:629` | `PreparedEdgeCopySourceFacts` via `prepare_edge_copy_source_facts` / `prepare_block_entry_parallel_copy_edge_source_facts` (`prepared_lookups.hpp:446`, `prepared_lookups.cpp:2975`, `prepared_lookups.cpp:3061`) | Present. Not the first migration target for this packet because call-boundary prepared effect consumption is narrower and within Step 2. |
| current-block join source | local clobber/read checks in `dispatch_publication.cpp:1384`, `dispatch_publication.cpp:1434` | `PreparedCurrentBlockJoinParallelCopySourceFacts` via `prepare_current_block_join_parallel_copy_source_facts` (`prepared_lookups.hpp:516`, `prepared_lookups.hpp:961`, `prepared_lookups.cpp:3123`) | Present but not visibly consumed in `dispatch_publication.cpp`; migration should wait for Step 3 after edge-copy source consumption is anchored. |
| typed stack-source publication | no direct `dispatch_publication.cpp` consume found in the Step 1 scan | `PreparedTypedStackSourcePublication` via `prepare_same_width_i32_stack_source_publication` (`prepared_lookups.hpp:601`, `prepared_lookups.hpp:742`, `prepared_lookups.cpp:2040`) | Present, but AArch64 consumption is missing or ambiguous from the scanned publication file. Keep as Step 4 target, not the first packet. |
| store-source publication | `dispatch_publication.cpp:1231`, `dispatch_publication.cpp:1250`, `dispatch_publication.cpp:1274` | `PreparedStoreSourcePublicationPlan` and `PreparedFixedFormalStoreSourcePublication` via `plan_prepared_store_source_publication` / `plan_prepared_fixed_formal_store_source_publication` (`publication_plans.hpp:163`, `publication_plans.hpp:272`, `publication_plans.hpp:286`, `publication_plans.hpp:308`) | Present and already consumed for fixed formal store-local publication. Broader store-source publication can remain a later Step 4 packet. |

## Suggested Next

First narrow migration packet: in `calls.cpp`, route one before-call explicit
register argument move path through the existing `PreparedCallBoundaryEffectPlan`
authority produced by `plan_prepared_call_boundary_effects`, while preserving
AArch64 register conversion, memory/immediate handling, instruction spelling,
and machine-record construction locally. Keep the packet limited to the
`BeforeCall` / `CallArgumentAbi` / register-destination subset already selected
by `classify_prepared_call_boundary_move`; do not widen into stack argument
copy lowering, publication ordering, or result-lane recording.

## Watchouts

- `PreparedCallBoundaryEffectPlan` has endpoint authority, but current
  AArch64 helpers still need `PreparedMoveBundle` / `PreparedMoveResolution`
  provenance for machine records. The next packet should thread effect
  authority without losing provenance.
- Avoid adding a new shared query until the existing `PreparedCallBoundaryEffectPlan`
  surface is proven insufficient.
- Publication ordering was not proven as an available authority in this Step 1
  scan. Treat any ordering gap as a blocker or separate handoff, not a local
  workaround.
- No clang-tools/AST lookup was needed for this mapping; targeted `rg` and
  direct source reads were sufficient.

## Proof

Command: `git diff --check -- todo.md`

Result: passed.
