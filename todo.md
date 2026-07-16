Status: Active
Source Idea Path: ideas/open/842_lir_restricted_first_class_value_unions.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire accepted universal boundary escape hatches

# Current Packet

## Just Finished

Completed plan.md Step 4 for the selected PHI value boundary as a bounded
no-code retirement check.

No safe PHI-only universal boundary escape hatch remains to delete in this
runbook slice. The selected PHI verifier, modeled-result, and printer authority
consumers have already migrated to `LirPhiOp.boundary_value_type`; the remaining
`LirPhiOp.type_str` use is compatibility/rendering parity text validated
against that carrier before rendering. Deleting `type_str` would require broader
receiver/backend/schema compatibility work outside the PHI-only packet.

## Suggested Next

Route active idea 842 to plan-owner for lifecycle disposition. The PHI-only
slice has delivered the restricted PHI boundary carrier and migrated the
selected verifier/printer consumers; remaining call, select, return, Raw-BIR,
and compatibility/schema work belongs to separate successors.

## Watchouts

PHI rendering intentionally still emits `LirPhiOp.type_str` as parity text, but
the printer now validates `LirPhiOp.boundary_value_type` before rendering. Do
not delete `LirPhiOp.type_str` in this PHI-only idea unless a future packet also
owns the receiver/backend compatibility and schema migration surface.

The Step 2 wrong-kind test covers runtime-text-only refs as the feasible local
surface for metadata-like/unbounded payload rejection. Partially parsed call
signature text and raw `args_str` payloads remain call-boundary concerns and
were not introduced into PHI.

## Proof

Step 4 proof command: `git diff --check`

Result: passed.

No `test_after.log` was written for Step 4 because no code changed in this
packet. The existing `test_after.log` remains from the prior Step 3 code proof.
