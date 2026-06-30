# RV64 Global Select Pointer Memory Residuals

Status: Closed
Type: Implementation and producer-boundary idea
Parent: `ideas/closed/429_rv64_pointer_address_materialization.md`
Source Evidence: `build/agent_state/429_step4_close_readiness/`
Owning Layer: RV64 global object data, direct-global select/return, and pointer-value memory with producer boundaries
Handoff Directory: `build/agent_state/429_step4_close_readiness/`
Closed Evidence:
- `build/agent_state/433_step1_residual_owner_reclassification/classification.md`
- `build/agent_state/433_step2_first_packet_selection/selection.md`
- `build/agent_state/433_step3_global_object_data/summary.txt`
- `build/agent_state/433_step4_residual_disposition/classification.md`

## Goal

Resolve the residual RV64 object-route failures left after bounded pointer-cast
materialization by separating coherent global/select/pointer-memory target
work from producer facts that need their own repair route.

## Why This Exists

Idea 429 closed the coherent scalar pointer-cast movement slice, but Step 4
close-readiness probes showed representative rows still failing for classes
that are broader than pointer casts or ordinary address materialization:
global object data, global load/store publication, direct-global select chains,
direct `bir.ret ptr @global`, terminator/select-publication behavior, and
pointer-value memory residuals.

These should not be silently folded back into pointer/address lowering. They
need a separate owner because some rows may be coherent RV64 target work while
others require prepared object-data, global-addressing, or terminator producer
facts first.

## In Scope

- Reclassify the residual rows from
  `build/agent_state/429_step4_close_readiness/` by first owner.
- Split global object-data failures, global pointer load/store publication,
  direct-global select chains, direct-global returns, terminator/select
  publication, and pointer-value memory residuals into coherent target work
  versus producer/prepared gaps.
- Add focused tests only for classes with explicit prepared authority.
- Preserve fail-closed diagnostics for missing object data, missing
  addressability, missing global layout, incoherent select-chain facts, and
  unsupported terminator publication.

## Out Of Scope

- Reopening bounded `inttoptr`/`ptrtoint` pointer-cast movement already closed
  by idea 429.
- Guessing object identity, global symbol addressability, relocation base,
  memory layout, or select-chain roots in RV64 lowering.
- Combining aggregate ABI, F128, scalar floating-point, integer div/rem, or
  unrelated call-publication work into this residual route.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Acceptance Criteria

- Residual rows are split into concrete owner buckets with representative
  evidence: global object data, global memory publication, direct-global
  select/return, terminator/select publication, pointer-value memory, and any
  producer/prepared gaps.
- Any implementation packet consumes explicit prepared facts and has focused
  backend tests for a semantic rule rather than a representative filename.
- Producer gaps are routed to separate ideas instead of being bypassed in RV64
  target lowering.
- The supervisor-delegated backend proof passes for any implementation slice.

## Completion Notes

- Step 1 reclassified residual rows from the 429 close-readiness handoff into
  owner buckets.
- Step 2 selected only the coherent selected global object-data packet, guarded
  by explicit prepared object-data facts.
- Step 3 implemented that packet; `20011019-1` now passes the RV64 object
  route and no longer reports the selected object-data diagnostic.
- Step 4 confirmed remaining representatives are separate residual families,
  not additional in-scope selected object-data work.
- Backend close gate used canonical `test_before.log` and `test_after.log`;
  both logs report 327 passed, 0 failed, and the non-decreasing regression
  guard passed.

## Follow-Up Ideas

- `ideas/open/438_prepared_pointer_value_memory_authority.md`
- `ideas/open/439_store_source_global_memory_publication_authority.md`
- `ideas/open/440_direct_global_return_select_chain_authority.md`
- `ideas/open/441_terminator_select_publication_authority.md`

## Reviewer Reject Signals

- Reject RV64 code that infers global object identity, symbol addressability,
  relocation base, memory layout, select-chain roots, or return pointer
  authority from filenames, raw BIR shape, or integer values.
- Reject testcase-shaped fixes keyed to `930930-1`, `20000622-1`,
  `20010329-1`, `20011019-1`, `20041112-1`, function names, block indexes, or
  one dump layout.
- Reject claiming progress through expectation rewrites, unsupported
  downgrades, allowlist edits, runtime-comparison changes, or pass/fail
  accounting changes.
- Reject helper renames, printer-only changes, or classification-only changes
  presented as capability progress when the same residual row still fails with
  the same owner.
- Reject broad memory-addressing rewrites that silently absorb prepared
  object-data, global-layout, select-chain, or terminator producer defects.

