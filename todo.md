Status: Active
Source Idea Path: ideas/open/355_rv64_prepared_object_shape_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Rejection Paths

# Current Packet

## Just Finished

Lifecycle activation created this executor scratchpad for Step 1. No executor
packet has run yet.

## Suggested Next

Start with Step 1 in `plan.md`: trace the RV64 prepared-object rejection path
and identify where the generic `unsupported prepared module shape` diagnostic
loses structured detail.

## Watchouts

- Do not implement RV64 capability repairs under this diagnostics idea.
- Do not define shared BIR/prepared semantics inside RV64 object emission.
- Do not use testcase-name matching or expectation downgrades as progress.

## Proof

No proof run for lifecycle-only activation.
