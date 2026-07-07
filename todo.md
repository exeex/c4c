Status: Active
Source Idea Path: ideas/open/550_rv64_scalar_fpr_residual_salvage.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Screen For F128 And Long-Double Leakage

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by screening all ten retained Step 1 residual rows
for F128, long-double, and helper leakage. The row-by-row result is saved in
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step2/screening.tsv`,
with supporting prepared dumps under per-case Step 2 directories. Two rows are
quarantined away from this lane's implementation decisions:
`src/20040709-1.c` and `src/ieee/20011123-1.c`. Seven rows remain ordinary
scalar/FPR candidates, and `src/va-arg-21.c` remains a variadic helper-lane
candidate rather than F128/long-double work.

## Suggested Next

Execute `plan.md` Step 3 by classifying first semantic owners for the retained,
non-quarantined rows: `src/20080529-1.c`, `src/930818-1.c`,
`src/loop-8.c`, `src/strct-pack-1.c`, `src/920618-1.c`,
`src/ieee/pr67218.c`, `src/pr23941.c`, and the helper-lane
`src/va-arg-21.c`.

## Watchouts

- Keep the F128/long-double quarantines outside implementation decisions:
  `src/20040709-1.c` and `src/ieee/20011123-1.c`.
- `src/va-arg-21.c` includes f128 declarations through libc headers, but the
  case-local prepared f128 carrier/helper sections are empty; its live issue is
  the variadic `va_start` helper path.
- The current row count differs from the source idea's historical 3/2/1 bucket
  count; use the Step 1 inventory as the current execution input unless the
  supervisor asks for a fresh full scan.
- Do not claim implementation progress from expectation rewrites, unsupported
  marker changes, or named-case shortcuts.
- Split coherent implementation follow-ups into separate `ideas/open/` files
  instead of widening this classification plan.

## Proof

Evidence-only packet; no build or root `test_after.log` was required. Proof
used the Step 1 inventory, current per-case logs, source inspection, and fresh
prepared dumps for all ten rows. Dump command template:
`build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu <source>`.
All ten dump return codes were `0`. Artifacts:
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step2/screening.tsv`,
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step2/summary.md`,
and per-case `dump-prepared-bir.*` files under the Step 2 directory.
