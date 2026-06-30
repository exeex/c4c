Status: Active
Source Idea Path: ideas/open/441_terminator_select_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Terminator/Select Publication Evidence

# Current Packet

## Just Finished

Activated the runbook for Step 1 of the terminator/select publication
authority plan.

## Suggested Next

Execute Step 1: inspect `build/agent_state/433_step4_residual_disposition/`
and `build/agent_state/440_step6_final_residual_disposition/`, with focus on
`20041112-1 bar.entry` where `%t6 = bir.ne ptr %t2, %t5` feeds
`bir.cond_br i32 %t6, block_4, block_5`. Classify terminator/select rows by
raw shape, prepared publication fact, source homes, compare operands, result
type, consumer boundary, authority state, and first missing producer or
consumer fact.

## Watchouts

- Keep this plan limited to terminator/select publication authority.
- Do not reopen direct-global return authority; closed idea 440 routes only the
  remaining `bar.entry` fused pointer compare branch here.
- Do not infer branch conditions, select results, compare operands, or
  terminator homes from raw BIR shape, block order, filenames, function names,
  or one dump layout.
- Keep missing or incoherent publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Activation-only validation:

```sh
git diff --check
```
