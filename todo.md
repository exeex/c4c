Status: Active
Source Idea Path: ideas/open/558_bir_call_metadata_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Direct-Call Representative And Classify Residual

# Current Packet

## Just Finished

Step 3 - Prove Direct-Call Representative And Classify Residual is complete for
the selected direct-call representative, `src/20000412-2.c`.

The supervisor-rerun RV64 backend-object proof still exits nonzero, but the
direct-call semantic producer admission failure is gone. The current `case.log`
reports downstream object lowering only:
`RISC-V backend object route unsupported prepared module shape:
unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering`.

No `semantic call family`, direct-call, or call-return failure remains in the
observed Step 3 logs for this row. `src/20000412-2.c` has moved off direct-call
semantic admission for producer-owned reasons and should stay out of this
source idea as downstream object lowering unless lifecycle opens a separate
route.

## Suggested Next

Ask plan-owner/lifecycle to advance toward Step 4 call-return inspection. The
runbook still requires call-return coverage before completion, with
`src/20050121-1.c` reserved as the call-return representative seed.

## Watchouts

Reject downstream RV64/MIR call inference, generic local-memory routing,
runtime/intrinsic repairs, expectation rewrites, unsupported-marker changes,
allowlist edits, runtime-comparison changes, and named-case shortcuts. The
current RV64 object-route failure for `src/20000412-2.c` is downstream and is
not a reason to broaden this source idea. The runbook must cover call-return
metadata before claiming the source idea is complete.

## Proof

Proof logs:

- `build/agent_state/558_step3_20000412.log`
- `build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log`

Commands:

- `ALLOWLIST=build/agent_state/558_step1_20000412.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/558_step3_20000412.log 2>&1`
  exited `1`, expected because the selected row still fails downstream.
- `git diff --check -- todo.md` passed.

Residual classification:

- Direct-call semantic admission: moved/resolved for this representative.
- Current owner: downstream RV64 object lowering.
- Current failure:
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
