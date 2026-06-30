Status: Active
Source Idea Path: ideas/open/448_array_aggregate_global_layout_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Non-Scalar Global Layout Evidence

# Current Packet

## Just Finished

Activated the runbook for Step 1 of the array/aggregate global layout
authority plan.

## Suggested Next

Execute Step 1: inspect `build/agent_state/446_step4_residual_disposition/`
with focus on `930930-1 @mem+792`, then classify non-scalar prepared
global-symbol memory rows by symbol, aggregate or array shape evidence, offset,
access width, alignment, range verdict, current layout authority, and first
missing producer fact. Record whether a bounded non-scalar layout-authority
producer packet exists or the exact blocker.

## Watchouts

- Keep this plan limited to array/aggregate global layout authority.
- Do not fold immediate global-store source encoding into this plan; that
  belongs to `ideas/open/447_immediate_global_store_source_encoding.md`.
- Do not infer array or aggregate global layout, object extent,
  addressability, offset meaning, or symbol identity in RV64 from raw BIR,
  symbol spelling, object labels, representative filenames, or dump shape.
- Keep non-scalar `layout_authority=unknown` fail-closed until producer facts
  are explicit.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Activation-only validation:

```sh
git diff --check
```
