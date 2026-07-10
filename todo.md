Status: Active
Source Idea Path: ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Object-Runtime BinaryInst Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: refresh focused object-runtime evidence for
`backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`, then
record whether the first owner is object-runtime `BinaryInst` support, RV64
object emission, relocation/object writer behavior, or missing prepared fact
publication.

## Watchouts

- Treat byval route/runtime rows from idea 659 as regression surfaces, not the
  implementation target.
- Do not reopen prepared byval call-boundary behavior without fresh focused
  evidence from the target row.
- Do not absorb generic RISC-V object-emission work unless the refreshed row
  proves that layer is the first owner.
- Do not use testcase names, final object bytes, fixed registers, or source
  shape as the object-runtime instruction authority.

## Proof

Activation-only lifecycle change; no build or regression proof required.
