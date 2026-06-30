Status: Active
Source Idea Path: ideas/open/450_select_result_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Result Branch Evidence

# Current Packet

## Just Finished

Activated the runbook for Step 1 of the select-result branch publication plan.

## Suggested Next

Execute Step 1: inspect `build/agent_state/441_step4_residual_disposition/`
and `build/agent_state/449_step4_residual_disposition/`, focusing on
`20010329-1` `unsupported_move_bundle_target_shape` with `root_is_select=yes`
rows for `%t22`, `%t36`, and `%t50`, plus `20000622-1` select-result branch
candidates such as `%t13/%t24` feeding `ne i32 ..., 0`. Classify each row by
root select identity, condition conversion, comparison shape, result value
home, move-bundle target, branch targets, publication authority, and first
missing producer, consumer, instruction-side, or materialization fact.

## Watchouts

- Keep this plan limited to select-result branch publication and move-bundle
  target materialization.
- Do not reopen pointer `eq/ne` or unsigned relational branch publication;
  those routes are closed by ideas 441 and 449.
- Do not fold stack-home branch operand materialization into this plan; that
  belongs to `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Do not infer select-result publication from raw select shape,
  `root_is_select=yes`, raw `ne i32 <select>, 0`, block order, filenames,
  function names, or one dump layout.
- Keep missing or incoherent select-result publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Activation-only validation:

```sh
git diff --check
```
