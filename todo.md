Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 12
Current Step Title: Prove Load Representative And Classify Residual

# Current Packet

## Just Finished

Step 12 - Prove Load Representative And Classify Residual completed the
representative proof already run during Step 11 for `src/20000314-1.c`.

RV64 representative result:

- `src/20000314-1.c` moved off semantic `load local-memory` admission.
- Current row failure is downstream prepared/object lowering:
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

## Suggested Next

Step 13 - Repair The Next Remaining Semantic Family.

Recommended next packet: inspect and repair the GEP representative
`src/20000717-4.c`, which is still classified as semantic `gep local-memory`.
Keep the downstream `src/20000314-1.c` object-lowering failure out of this
local-memory semantic producer route unless the supervisor opens a separate
route for prepared/object instruction support.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

The `src/20000314-1.c` row is no longer a `load local-memory` semantic
admission failure. Its current failure is downstream object-route support.

The `src/20001026-1.c` row is no longer a store local-memory semantic admission
failure. Its current failure is downstream object-route support, so do not keep
classifying that row as an unchanged BIR producer gap.

Existing deliberate fail-closed coverage around casted byte-pointer opaque
`i32` access was preserved. This packet admits only the byte-compatible
`inttoptr` load fact needed by the selected representative and does not add a
typed opaque local-integer pointer recovery path.

## Proof

Proof log: `test_after.log`.

Commands run:

- Already present from Step 11:
  `ALLOWLIST=build/agent_state/557_step12_20000314.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Result: nonzero with `0/1` passed; the row moved from semantic
  `load local-memory` admission to downstream `unsupported_instruction_fragment`.

Inspected case log:
- `build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log`
