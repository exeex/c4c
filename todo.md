Status: Active
Source Idea Path: ideas/open/440_direct_global_return_select_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Direct-Global Evidence

# Current Packet

## Just Finished

Activated the runbook for Step 1 of the direct-global return/select-chain
authority plan.

## Suggested Next

Execute Step 1: inspect `build/agent_state/433_step4_residual_disposition/`
and `build/agent_state/447_step4_residual_disposition/` for direct-global
return and select-chain rows, especially `20041112-1`. Classify each row by
raw BIR shape, prepared fact, global symbol, value home or terminator use,
current authority state, and first missing producer or consumer fact.

## Watchouts

- Keep this plan limited to direct-global return/select-chain authority.
- Do not fold general terminator/select publication into this plan; that
  belongs to `ideas/open/441_terminator_select_publication_authority.md`.
- Do not infer return authority or select-chain roots from raw
  `bir.ret ptr @global`, symbol spelling, select shape, testcase names, or one
  dump layout.
- Keep missing or incoherent direct-global authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log` or `test_after.log` during activation.

## Proof

Activation-only validation:

```sh
git diff --check
```
