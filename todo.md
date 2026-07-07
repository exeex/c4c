Status: Active
Source Idea Path: ideas/open/550_rv64_scalar_fpr_residual_salvage.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce Current Residual Rows

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by extracting the current scalar-compare,
floating-cast, and variadic-helper residual rows from the mutable RV64
gcc_torture backend summary and per-case logs. The current retained inventory
is saved under
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step1/` and has drifted
from the older six-row source count to ten direct matches: four
`unsupported_scalar_compare_publication`, five `unsupported_floating_cast`, and
one `unsupported_variadic_helper_lowering`.

## Suggested Next

Execute `plan.md` Step 2 by screening the ten retained rows in
`residual-inventory.tsv` for F128, long-double, and soft-float/helper leakage.
Treat the two `src/ieee/...` floating-cast rows as likely high-priority
screening candidates, but do the full row-by-row check before proposing
implementation follow-up work.

## Watchouts

- Keep F128 and long-double helper cases quarantined outside this lane.
- The current row count differs from the source idea's historical 3/2/1 bucket
  count; use the Step 1 inventory as the current execution input unless the
  supervisor asks for a fresh full scan.
- Do not claim implementation progress from expectation rewrites, unsupported
  marker changes, or named-case shortcuts.
- Split coherent implementation follow-ups into separate `ideas/open/` files
  instead of widening this classification plan.

## Proof

Evidence-only packet; no build or route proof was required. Current evidence
was extracted from
`build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and the per-case
logs referenced there. Artifacts:
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step1/current-log-direct-matches.tsv`,
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step1/residual-inventory.tsv`,
and
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step1/summary.md`.
