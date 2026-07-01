Status: Active
Source Idea Path: ideas/open/423_rv64_runtime_mismatch_triage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Runtime Mismatch Rows

# Current Packet

## Just Finished

Activation initialized this execution state for `plan.md` Step 1.

## Suggested Next

Execute Step 1: inventory current RV64 gcc_torture runtime mismatch rows from
the freshest matching summary and per-case logs, excluding fail-closed
unsupported compile/codegen rows.

## Watchouts

- This is a triage runbook, not an implementation route.
- Do not weaken runtime comparison, expected output, allowlists, unsupported
  markers, or pass/fail accounting.
- Do not let F128 drive the route unless runtime row evidence proves it owns a
  recurring mismatch family.
- If a row needs missing BIR/prepared authority, split a producer-owned
  follow-up idea instead of inferring facts in RV64.

## Proof

Lifecycle activation only. No build or test proof required yet.
