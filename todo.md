Status: Active
Source Idea Path: ideas/open/558_bir_call_metadata_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Inspect And Repair Call-Return Metadata

# Current Packet

## Just Finished

Step 3 - Prove Direct-Call Representative And Classify Residual was completed
for the selected direct-call representative, `src/20000412-2.c`.

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

Execute Step 4 - Inspect And Repair Call-Return Metadata.

Inspect `src/20050121-1.c` as the call-return representative seed, capture the
current call-return row log or BIR dump, and trace returned call-value metadata
through BIR call emission and semantic admission. Add focused BIR coverage for
the call-return result metadata boundary before repairing producer publication.

Supervisor proof seed for the packet:

- `src/20050121-1.c`

Suggested narrow RV64 representative command:

- `printf '%s\n' src/20050121-1.c > build/agent_state/558_step4_20050121.allowlist`
- `ALLOWLIST=build/agent_state/558_step4_20050121.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`

## Watchouts

Reject downstream RV64/MIR call inference, generic local-memory routing,
runtime/intrinsic repairs, expectation rewrites, unsupported-marker changes,
allowlist edits, runtime-comparison changes, and named-case shortcuts. The
current RV64 object-route failure for `src/20000412-2.c` is downstream and is
not a reason to broaden this source idea. The runbook must cover call-return
metadata before claiming the source idea is complete.

For Step 4, do not infer call-return result facts in prepared/RV64 consumers.
If `src/20050121-1.c` has already moved to a different owner, inspect at least
one current call-return row before asking lifecycle to split or advance.

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
