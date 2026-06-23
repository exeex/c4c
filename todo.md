Status: Active
Source Idea Path: ideas/open/311_rv64_ordinary_control_expression_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Representative Failure Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: normalize representative evidence for
`src/00008.c`, `src/00030.c`, and `src/00105.c`, then identify the first
semantic RV64 emission boundary to repair.

## Watchouts

- Do not implement filename-shaped logic or c-testsuite-specific shortcuts.
- Do not downgrade supported-path expectations or mark cases unsupported.
- Keep stack/local address materialization and external-stub policy separate
  unless they directly block ordinary control/expression completion proof.

## Proof

Lifecycle-only activation. No build or CTest proof required.
