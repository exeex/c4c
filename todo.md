# Current Packet

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Fresh Backend Regex Inventory

## Just Finished

Lifecycle activation created the active backend-regex inventory runbook for
Step 1. No executor packet has run yet.

## Suggested Next

Supervisor should delegate Step 1 with the backend-regex command to run from
the main build tree, expected to be shaped like:

```bash
ctest --test-dir build -j10 -R backend --output-on-failure
```

The executor should record selected/passed/failed/timed-out counts, list
non-passing tests, separate local backend tests from external AArch64
c-testsuite tests, and confirm that `00173` remains passing.

## Watchouts

- This is an inventory and classification packet, not an implementation
  packet.
- Do not touch implementation files, tests, `ideas/closed/`, `review/`
  artifacts, logs, runner behavior, timeout policy, CTest registration,
  proof-log policy, expectations, or unsupported classifications.
- Do not reactivate stale parked focused ideas from their header alone; use
  current first-bad-fact evidence.
- Treat the 00173 path as completed unless fresh evidence shows a new,
  distinct first bad fact.

## Proof

Not run. Activation is lifecycle-only.
