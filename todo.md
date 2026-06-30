Status: Active
Source Idea Path: ideas/open/427_rv64_scalar_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Evidence And Select A First Owned Shape

# Current Packet

## Just Finished

Activation created the runbook from `ideas/open/427_rv64_scalar_select_join_materialization.md`.

## Suggested Next

Execute Step 1: inspect the select/join evidence rows, confirm the first coherent prepared scalar `bir.select` or join-transfer row family, and record the first implementation packet plus supervisor-delegated proof command before code changes.

## Watchouts

- Do not implement code during lifecycle activation.
- Do not edit expectations, allowlists, unsupported markers, runtime comparison, or pass/fail accounting.
- Keep F128, call-adjacent publication, aggregate ABI, pointer provenance, and global memory repair out of this route.
- Reject filename-shaped fixes for representative gcc_torture rows.

## Proof

Lifecycle activation only. Run `git diff --check` before returning.
