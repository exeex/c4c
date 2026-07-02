Status: Active
Source Idea Path: ideas/open/548_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Global-Data Ownership

# Current Packet

## Just Finished

Completed plan Step 1 by reconstructing current representative evidence for
the three infrastructure buckets with a seven-row RV64 gcc-torture allowlist.

Fresh representative outcomes:

- `unsupported_global_data`
  - `src/20000412-1.c`:
    `unsupported_global_data: prepared selected object-data contract
    status=unsupported_but_coherent object_label_id=2 object_size_bytes=1656
    emitted_byte_count=0 zero_fill_byte_count=0`.
  - `src/20001121-1.c`:
    `unsupported_global_data: RV64 object route supports only 1-, 2-, 4-, and
    8-byte prepared global memory accesses`.
  - First-owner hint: prepared object-data facts are present enough to select
    the object route; later classification should separate prepared selected
    object-data contract support from RV64 object emission consumption in
    `src/backend/mir/riscv/codegen/object_emission.cpp`.
  - Evidence sufficiency for Step 2: enough.
- `unsupported_stack_frame`
  - `src/20000603-1.c`:
    `unsupported_stack_frame: RV64 object route does not support non-GPR
    prepared callee-saved register save slots (fpr:fs1)`.
  - `src/20030209-1.c`:
    same `fpr:fs1` non-GPR prepared callee-saved save-slot diagnostic.
  - First-owner hint: prepared frame/callee-saved facts exist and name FPR
    slots; later classification should decide whether ordinary-C scope can
    support RV64 FPR callee-saved frame emission or whether these rows belong
    outside the immediate GPR stack-frame lane.
  - Evidence sufficiency for Step 3: enough.
- `unsupported_prepared_move_bundle_classification`
  - `src/20010224-1.c`:
    stack-source stack-destination conversion-adjacent move rejected by
    `diagnostic_owner=prepared_move_bundle_classifier`,
    `fragment_status=producer_classification_rejected_stack_source_stack_destination_conversion_adjacent_move`,
    in function `ba_compute_psd`.
  - `src/pr87623.c`:
    same classifier rejection shape in function `a_or_b_different`.
  - First-owner hint: producer-side prepared move-bundle classification is
    rejecting conversion-adjacent stack-to-stack moves before RV64 lowering;
    later classification should inspect classifier authority rather than
    consume the move in RV64 by inference.
  - Evidence sufficiency for Step 4: enough.

Rows that moved or are missing from intended buckets:

- `src/930513-2.c` now passes the delegated RV64 object-route backend
  allowlist row and is no longer current evidence for an infrastructure
  bucket.

## Suggested Next

Execute Step 2 by classifying the two current `unsupported_global_data`
representatives from Step 1:

- `src/20000412-1.c`: determine whether the
  `prepared selected object-data contract status=unsupported_but_coherent`
  diagnostic means prepared still owes an object-data contract capability, or
  whether RV64 object emission should consume the coherent prepared payload.
- `src/20001121-1.c`: determine whether prepared facts are already sufficient
  and the first owner is RV64 object-route global memory access emission for
  widths outside 1, 2, 4, and 8 bytes.

Record the prepared-contract versus RV64 object-route sub-buckets, exact log
paths, and whether the evidence is implementation-ready for one or more
follow-up source ideas.

## Watchouts

- This is a review/classification plan. Do not implement prepared or RV64
  lowering inside this active idea.
- `src/20000412-1.c` and `src/20001121-1.c` are both global-data bucket rows,
  but they expose different first-owner questions: object-data contract status
  versus RV64 supported access widths.
- The current stack-frame evidence is FPR callee-saved slot shaped
  (`fpr:fs1`), so Step 3 should explicitly screen FPR scope before treating it
  as ordinary GPR frame work.
- The move-bundle rows already name `prepared_move_bundle_classifier` as
  diagnostic owner; avoid converting them into RV64 consumer work before
  classifier authority is reviewed.
- Do not change expectations, unsupported markers, allowlists, or pass/fail
  accounting as evidence of progress.

## Proof

Proof command:

```sh
printf '%s\n' src/20000412-1.c src/20001121-1.c src/930513-2.c src/20000603-1.c src/20030209-1.c src/20010224-1.c src/pr87623.c > build/agent_state/548_step1_infrastructure_evidence.allowlist && ALLOWLIST=build/agent_state/548_step1_infrastructure_evidence.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/548_step1_infrastructure_evidence.log 2>&1
```

Result: exited `1`, acceptable for this evidence packet. The run produced
`total=7 passed=1 failed=6`.

Traceable logs:

- Aggregate:
  `build/agent_state/548_step1_infrastructure_evidence.log`
- Allowlist:
  `build/agent_state/548_step1_infrastructure_evidence.allowlist`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20000412-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20001121-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_930513-2.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20000603-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20030209-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_20010224-1.c/case.log`
- Per-case:
  `build/rv64_gcc_c_torture_backend/src_pr87623.c/case.log`

No root-level `test_after.log` was written because the delegated packet
explicitly owned only the Step 1 evidence log artifacts and forbade touching
root-level `test_before.log` or `test_after.log`.
