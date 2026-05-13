Status: Active
Source Idea Path: ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Existing Record Coverage

# Current Packet

## Just Finished

Activation initialized this execution state from `ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md`.

## Suggested Next

Begin Step 1 by inspecting the existing AArch64 `module` record coverage against `ALLOCATION_CONTRACT.md`, then record concrete gaps and the smallest next code/docs targets here.

## Watchouts

- Do not select or emit final `ldr`, `str`, `mov`, call, return, prologue, or assembly forms.
- Do not treat prepared stack offsets as final frame-layout authority.
- Do not recover allocation facts from rendered names, printed BIR, LIR text, parsed assembly, or legacy examples.
- Split missing shared prepared carriers into a separate open idea instead of patching around them locally.

## Proof

Lifecycle activation only. No build proof required yet.
