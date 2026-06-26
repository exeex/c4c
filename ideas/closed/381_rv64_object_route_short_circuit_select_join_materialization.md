# RV64 Object Route Short-Circuit Select Join Materialization

Status: Closed
Type: Repair idea
Parent: `ideas/closed/380_rv64_object_route_short_circuit_call_argument_reload.md`

Closed: 2026-06-26

## Goal

Repair the RV64 object-route short-circuit select/join materialization owner
exposed after idea 380 advanced `src/20000112-1.c` past the call-argument
prior-preservation failure.

## Why This Exists

Idea 380 repaired the repeated-call argument preservation gap in
`special_format`: the original incoming `fmt` pointer is now populated into a
callee-saved home before the first clobbering `strchr`, and later calls reload
from that home.

The representative now aborts later instead of failing under the original NULL
call-argument owner. The classification artifact
`build/agent_state/380_residual_followup_20000112.classification.txt` reports
that prepared BIR contains phi-edge immediate materialization bundles for
short-circuit skip edges, but RV64 object emission still reads the RHS result
home `s2` at logic-end joins even on skip paths where the RHS call did not run.

This is a distinct select/join materialization problem in object emission, not
more call-argument preservation work.

## In Scope

- Audit the prepared BIR, value-home, phi-edge move bundle, and object emission
  evidence for the first select/join materialization mismatch after idea 380.
- Identify where RV64 object emission loses or ignores skip-edge immediate
  materialization for short-circuit joins.
- Implement the smallest semantic object-route repair that publishes the
  authoritative select/join value on every incoming edge before join consumers
  read it.
- Add focused backend coverage for the admitted short-circuit select/join
  materialization shape and for adjacent unsupported or ambiguous edge
  publication shapes.
- Rerun `src/20000112-1.c` and record whether it passes or advances to another
  distinct owner.

## Out of Scope

- Reopening idea 380 call-argument prior-preservation once its repaired
  behavior remains green.
- Reopening idea 379's earlier select/publication repair unless new evidence
  proves it regressed.
- Testcase-name handling for `src/20000112-1.c`, `special_format`, a specific
  `strchr` call, exact C expression spelling, block labels, value ids,
  instruction indexes, registers, or object addresses.
- Broad CFG reconstruction, register-allocation replacement, assembler/object
  replacement, full ABI redesign, byval aggregate homes, aggregate `va_arg`,
  globals, or strings unless the select/join audit proves they are the direct
  owner.
- Weakening runtime expectations, external allowlists, supported-path
  contracts, or focused backend coverage.

## Acceptance Criteria

- The first short-circuit select/join materialization failure is documented
  from prepared/object-route evidence.
- Focused backend tests prove the admitted skip-edge/join materialization
  behavior and keep neighboring unsupported shapes fail-closed.
- Existing focused backend coverage for ideas 379 and 380 remains green.
- `src/20000112-1.c` is rerun and either passes or advances to a documented
  distinct next owner after the select/join repair.

## Starting Evidence

- `build/agent_state/380_residual_followup_20000112.classification.txt`
- `build/agent_state/380_residual_followup_20000112.special_format_disasm.txt`
- `build/agent_state/380_residual_followup_20000112.prepared_bir.txt`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `test_after.log`

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts for `src/20000112-1.c`, `special_format`, a
  specific `strchr` call, exact C expression spelling, block labels, value ids,
  instruction indexes, physical registers, object addresses, or log text.
- Reject fixes that only rewrite diagnostics, expected classifications,
  allowlists, or runtime contracts while preserving the skip-edge read from an
  unmaterialized RHS result home.
- Reject claiming progress from helper renames, comment updates, or
  classification-only changes without a semantic select/join or edge
  publication repair.
- Reject broad register-allocation, CFG, or ABI rewrites unless the audit
  proves the current prepared phi-edge/value-home contract cannot express the
  required materialization.
- Reject weakening or deleting focused idea 379/380 coverage to make the new
  owner appear isolated.

## Closure Note

Idea 381 is accepted as complete. The RV64 object route now materializes
supported short-circuit select/join edge producers before join consumers read
them, and the focused backend proof for the admitted behavior plus idea 379 and
idea 380 coverage remained green.

Closure evidence:

- Step 2 implementation commit: `d2d290ea Materialize RV64 select edge producers`
- Step 3 representative rerun commit:
  `f0e92a52 [todo_only] record 20000112 select join rerun`
- Representative `src/20000112-1.c` passed through the RV64 GCC torture backend
  allowlist path after the repair.
- Close-time regression guard passed on matching focused 13-test CTest logs:
  before passed=13 failed=0 total=13; after passed=13 failed=0 total=13.
