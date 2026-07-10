# RV64 Pointer-Local Lowering Route Runtime

Status: Closed
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md`
- `build/agent_state/657_step3_representative_pointer_value_store/summary.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: RV64 pointer-local lowering for prepared local-address,
pointer-step, and store-source cases
Queue Order: 60
Proof Surface: current baseline rows 120, 122, 127, 192, 193, and 195 from
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Completion Notes

Closed after Step 1 evidence refresh found the complete focused
pointer-local route/runtime proof surface already passing:

- `backend_codegen_route_riscv64_loop_carried_pointer_postincrement`
- `backend_codegen_route_riscv64_duff_fallthrough_pointer_update_producers`
- `backend_codegen_route_riscv64_i16_local_array_select_store`
- `backend_rv64_runtime_riscv64_loop_carried_pointer_postincrement`
- `backend_rv64_runtime_riscv64_duff_fallthrough_pointer_update_producers`
- `backend_rv64_runtime_riscv64_i16_local_array_select_store`

No implementation boundary remained to select without inventing work. The
focused before/after regression guard for this no-code lifecycle close was
6/6 passing before and after, with zero new failing tests.

## Goal

Repair the current RV64 pointer-local route/runtime family without reopening
the already-superseded `loop-2e.c` representative failure as the first owner.

## Why This Exists

Step 2 classified six failed rows around loop-carried pointer
postincrement, Duff fallthrough pointer-update producers, and i16 local-array
select/store lowering. Newer 2026-07-10 04:18 evidence says idea 657's
representative `loop-2e.c` object-runtime comparison passes, so this idea
must start from the remaining current baseline rows rather than from the older
657 Step 4 mismatch.

## In Scope

- Refresh focused route, object, disassembly, and runtime evidence for the six
  pointer-local rows.
- Prove the first owner among prepared local-address publication, pointer-step
  producer facts, store-source selection, and RV64 consumer lowering.
- Repair a general pointer-local lowering rule only after the source and
  destination facts are visible at the consumer program point.
- Preserve the 2026-07-10 04:18 representative 657 pass unless fresh evidence
  proves a regression.

## Out Of Scope

- Reopening idea 657's `loop-2e.c` indirect-store/postincrement route as the
  first owner without newer failing evidence.
- Prepared stack-destination fan-in authority from ideas 647 or 655.
- Byval/prepared call-boundary aggregate handling.
- Prepared object-data static storage, callee-saved GPR, packed member offset,
  AArch64, CLI, or LLVM torture work.
- Test expectation rewrites, unsupported-marker changes, allowlists, runtime
  policy changes, timeout changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence distinguishes the remaining pointer-local rows from the
  repaired 657 representative state.
- The selected repair uses semantic/prepared/RV64 facts for pointer local
  source, destination, and update order, not testcase or filename matching.
- The focused pointer-local route/runtime subset passes or fails closed with a
  precise diagnostic at the proven owner.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Reviewer Reject Signals

- Reject using the older 2026-07-10 04:12 657 mismatch as authoritative while
  the newer 04:18 representative pass remains current.
- Reject named-case shortcuts for loop-carried, Duff fallthrough, i16
  local-array, `loop-2e.c`, pointer variable names, or fixed temporary ids.
- Reject storing through a local pointer home when facts prove the semantic
  destination is an external pointee.
- Reject unsupported-marker downgrades, expectation rewrites, allowlist edits,
  helper renames, or classification-only edits claimed as capability progress.
- Reject broad changes that absorb byval, object-data static storage,
  AArch64, CLI, or LLVM torture owners into this pointer-local route.
