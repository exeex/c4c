Status: Active
Source Idea Path: ideas/open/447_immediate_global_store_source_encoding.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Immediate Global-Store Evidence

# Current Packet

## Just Finished

Activated the runbook for Step 1 of the immediate global-store source encoding
plan.

## Suggested Next

Execute Step 1: inspect `build/agent_state/439_step4_residual_disposition/`
with focus on immediate-valued global stores such as
`20041112-1 main.entry.0`, then classify rows by destination global, offset,
store width, current layout authority, immediate value, current source
encoding, and first missing producer fact. Record whether a bounded
immediate-source producer packet exists or the exact blocker.

## Watchouts

- Keep this plan limited to immediate global-store source encoding.
- Do not reopen scalar or integer-array global layout authority; those are
  closed by ideas 446 and 448.
- Do not infer immediate values or source publication in RV64 from raw BIR
  store shape, testcase names, block labels, symbol names, or one dump layout.
- Keep `source=<none>` and `source_producer=unknown` fail-closed until
  producer facts are explicit.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Activation-only validation:

```sh
git diff --check
```
