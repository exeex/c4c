Status: Active
Source Idea Path: ideas/open/421_rv64_instruction_fragment_bucket_classification.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Evidence Source And Bucket Inputs

# Current Packet

## Just Finished

Activation created this executor-compatible packet skeleton for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: confirm the evidence source for
`unsupported_instruction_fragment` rows and record any missing or stale bucket
inputs in the handoff docs.

## Watchouts

- Do not implement RV64 lowering in this classification runbook.
- Do not make F128 the primary route.
- Do not weaken expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Do not classify missing BIR/prepared producer facts as RV64 lowering work.

## Proof

Not run. Activation is lifecycle-only and changed only `plan.md` and `todo.md`.
