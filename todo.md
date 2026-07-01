Status: Active
Source Idea Path: ideas/open/423_rv64_runtime_mismatch_triage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Runtime Mismatch Rows

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by inventorying the freshest RV64 gcc_torture
runtime mismatch rows in
`docs/rv64_gcc_torture_post_contract/runtime_mismatch_inventory.md`.
The inventory records the `.full.tsv`/`.full.txt` provenance, runtime-only
extraction criteria, row counts, representative case IDs, exit-mode
partitions, and per-case source log paths. Unsupported compile/codegen and
fail-closed contract rows are explicitly excluded.

## Suggested Next

Execute Step 2 by reproducing a small representative runtime mismatch set:
abort rows such as `src/20000314-2.c`, `src/pr38533.c`, and `src/pr83298.c`;
segfault rows such as `src/20080506-2.c`, `src/pr43008.c`, and
`src/zerolen-1.c`; and the exit-255 row `src/pr81503.c`.

## Watchouts

- This is a triage runbook, not an implementation route.
- Do not weaken runtime comparison, expected output, allowlists, unsupported
  markers, or pass/fail accounting.
- Do not let F128 drive the route unless runtime row evidence proves it owns a
  recurring mismatch family.
- If a row needs missing BIR/prepared authority, split a producer-owned
  follow-up idea instead of inferring facts in RV64.
- The current runtime inventory is limited to rows whose case logs contain
  `[RV64_BACKEND_RUNTIME_MISMATCH]`; unsupported compile/codegen failures are
  intentionally out of scope even when they dominate whole-scan counts.
- No matching timestamped top-level `.full.log` was present for the 2026-07-01
  full artifacts, so Step 2 should preserve any representative reproduction
  logs it creates.

## Proof

Passed:
`git diff --check -- docs/rv64_gcc_torture_post_contract/runtime_mismatch_inventory.md todo.md`.
Proof output is preserved in `test_after.log`.
