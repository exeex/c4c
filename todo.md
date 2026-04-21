# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Baseline The Current Debug Ladder Against `00204.c`
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Lifecycle switch only. Active planning state moved from idea 60 to idea 67 and
the canonical runbook was regenerated from
`ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md`.

## Suggested Next

Run the current backend debug ladder on `00204.c` or the nearest reduction
that preserves the same observability problem. Record which questions are
already answered by `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, and
`--trace-mir`, then isolate one concrete gap for the first executor packet.

## Watchouts

- Do not treat one green backend case as completion for idea 67. The owned
  goal is debug-surface quality, not matcher coverage.
- Do not add testcase-shaped debug prints or one-off logging as the main
  result.
- Keep the next packet scoped to one observability seam: prepared-delta
  summary, x86 rejection diagnostics, or large-case focus/filtering.

## Proof

Lifecycle switch only. No implementation proof run was performed in this
packet.
