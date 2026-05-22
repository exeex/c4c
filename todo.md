Status: Active
Source Idea Path: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Duplicate-Label Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
the synthetic select label uniqueness refresh runbook.

## Suggested Next

Run Step 1: rebuild the focused subset selected by the supervisor, include
`c_testsuite_aarch64_backend_src_00143_c`, and inspect generated assembly for
duplicate `.Lselect_mat_*` definitions when available.

## Watchouts

- Do not claim runtime `00143` failures as synthetic-label progress unless
  duplicate synthetic labels are still present.
- Do not reopen block-label ordering or scalar-cast source publication without
  fresh first-bad-fact evidence.
- Do not weaken expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, or assembler/test
  contracts.

## Proof

Not run by plan owner; lifecycle activation only.
