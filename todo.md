Status: Active
Source Idea Path: ideas/open/550_rv64_scalar_fpr_residual_salvage.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Classify First Semantic Owners

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by classifying first semantic owners for the eight
retained non-quarantined rows. The row-by-row result is saved in
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv`,
with a narrative summary in
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md`.

First-owner groups:

- Scalar compare publication: `src/20080529-1.c`, `src/930818-1.c`,
  `src/loop-8.c`, and `src/strct-pack-1.c` all first hit RV64 object-emission
  publication for non-terminator FPR compare results in prepared GPR homes.
- Floating cast: `src/920618-1.c`, `src/ieee/pr67218.c`, and
  `src/pr23941.c` first hit RV64 object `CastInst` lowering for ordinary
  F32/F64 cast forms, not F128/long-double helper work.
- Variadic helper: `src/va-arg-21.c` first hits RV64 `va_start` helper lowering
  where prepared helper operands place the destination va_list address in stack
  slots instead of prepared GPR homes.

No retained group shares implementation ownership with another group. The
scalar compare publication and floating-cast groups are coherent enough for
separate follow-up ideas. The variadic helper row is coherent only as a
separate helper-focused idea; this scalar/FPR lane should not mix it into a
compare or cast implementation packet.

## Suggested Next

Have the supervisor route lifecycle follow-up from this classification. The
next coherent implementation packet should target only one owner group, or the
plan owner should split the groups into separate `ideas/open/` follow-ups if
the current classification runbook is considered exhausted.

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
- Split coherent implementation follow-ups into separate `ideas/open/` files
  instead of widening this classification plan.

## Proof

Evidence-only packet; no build or root `test_after.log` was required. Proof
used Step 1 `residual-inventory.tsv`, Step 2 `screening.tsv`, the Step 2
prepared dumps, current per-case route logs, and a targeted read of the RV64
object-emission diagnostic surfaces. Artifacts:
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv` and
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md`.
