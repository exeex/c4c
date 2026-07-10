Status: Active
Source Idea Path: ideas/open/674_rv64_object_terminator_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Row 176 Terminator Evidence

# Current Packet

## Just Finished

Lifecycle activation created the runbook for idea 674 and positioned execution
at Step 1.

## Suggested Next

Execute Step 1: refresh row 176 evidence for
`backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`, capture
the current `unsupported_terminator_fragment` boundary, and name the prepared
branch facts and RV64 object terminator owner before any implementation edit.

## Watchouts

- Keep row 139 out of this route; it belonged to idea 673.
- Keep row 256 `backend_riscv_object_emission` as a guard surface.
- Do not edit expectations, unsupported markers, allowlists, runtime policy, or
  baseline accounting.
- Reject testcase-shaped lowering tied only to the row 176 test name.

## Proof

No validation was run for lifecycle-only activation.
