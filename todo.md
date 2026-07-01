Status: Active
Source Idea Path: ideas/open/443_rv64_prepared_value_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-establish Prepared Value Evidence

# Current Packet

## Just Finished

Activation initialized this execution state for `plan.md` Step 1.

## Suggested Next

Execute Step 1: re-establish the prepared value evidence for `src/pr81503.c`
from the runtime triage handoff, then identify the exact RV64 consumer edges
that must stop using stale operand registers.

## Watchouts

- Consume prepared value authority directly; do not infer producer facts from
  testcase names, raw source shape, register accidents, or qemu exit codes.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Keep frame-slot call-argument publication in the separate idea `444`.
- If the evidence proves missing prepared producer authority, stop and report
  the producer gap instead of reconstructing it in RV64.

## Proof

Lifecycle activation only. No build or test proof required yet.
