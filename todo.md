Status: Active
Source Idea Path: ideas/open/550_rv64_scalar_fpr_residual_salvage.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Split Coherent Follow-Ups Or Record No-Implementation

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by splitting the retained actionable owner groups
from Step 3 into separate open follow-up ideas. The row-by-row owner evidence
remains saved in
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv`,
with a narrative summary in
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md`.

Follow-up ideas created:

- `ideas/open/580_rv64_scalar_compare_publication.md` for
  `src/20080529-1.c`, `src/930818-1.c`, `src/loop-8.c`, and
  `src/strct-pack-1.c`.
- `ideas/open/581_rv64_ordinary_floating_cast_lowering.md` for
  `src/920618-1.c`, `src/ieee/pr67218.c`, and `src/pr23941.c`.
- `ideas/open/582_rv64_va_start_stack_backed_destination.md` for
  `src/va-arg-21.c`.

No implementation follow-up was created for the Step 2 quarantined
F128/long-double rows `src/20040709-1.c` and `src/ieee/20011123-1.c`; those
rows are outside this scalar/FPR/helper salvage lane's implementation scope.
Each created idea has its own reviewer reject signals and does not mix scalar
compare, floating cast, and variadic helper work.

## Suggested Next

Proceed to `plan.md` Step 5 closure-readiness evaluation for
`ideas/open/550_rv64_scalar_fpr_residual_salvage.md`. The likely lifecycle
question is whether this classification/splitting source idea is now complete
because all retained rows have reproduction status, F128 screening,
first-owner classification, and separate follow-up ideas where implementation
is justified.

## Watchouts

- Keep the F128/long-double quarantines outside implementation decisions:
  `src/20040709-1.c` and `src/ieee/20011123-1.c`.
- Do not implement scalar compare publication, floating casts, and va_start
  helper lowering in one packet; they are distinct RV64 object-emission
  surfaces.
- `src/loop-8.c` and `src/strct-pack-1.c` include select-chain/select-carrier
  evidence, but Step 3 classifies them under scalar compare publication rather
  than as a separate owner.
- `src/va-arg-21.c` includes f128 declarations through libc headers, but the
  case-local prepared f128 carrier/helper sections are empty; its live issue is
  the variadic `va_start` helper destination-address home.
- Do not claim implementation progress from expectation rewrites, unsupported
  marker changes, or named-case shortcuts.
- Do not implement the new follow-up ideas under this classification runbook;
  the supervisor should switch lifecycle state when choosing one.

## Proof

Lifecycle-only split packet; no build or root `test_after.log` was required.
Proof used Step 1 `residual-inventory.tsv`, Step 2 `screening.tsv`, the Step 2
prepared dumps, current per-case route logs, and Step 3 owner artifacts:
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv` and
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md`.
