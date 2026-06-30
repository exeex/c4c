Status: Active
Source Idea Path: ideas/open/446_global_memory_layout_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Global Layout Authority Evidence

# Current Packet

## Just Finished

Activated the runbook for Step 1 of the global memory layout authority
publication plan.

## Suggested Next

Execute Step 1: inspect `build/agent_state/439_step4_residual_disposition/`
and classify prepared global-symbol memory rows by symbol, offset, access
width, range verdict, current layout authority, and first missing producer
fact. Record whether a bounded layout-authority producer packet exists or the
exact blocker.

## Watchouts

- Keep this plan limited to global layout authority publication.
- Do not fold immediate store-source encoding into this plan; that belongs to
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Do not infer global layout, object extent, addressability, offset meaning,
  or symbol identity in RV64 from raw BIR, symbol spelling, object labels,
  representative filenames, or dump shape.
- Keep `layout_authority=unknown` fail-closed until producer facts are
  explicit.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Activation-only validation:

```sh
git diff --check
```
