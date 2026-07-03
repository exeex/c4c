# Current Packet

Status: Active
Source Idea Path: ideas/open/564_rv64_fpr_callee_saved_frame_slots.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect FPR Callee-Saved Boundary

## Just Finished

Activated `ideas/open/564_rv64_fpr_callee_saved_frame_slots.md` into
`plan.md` and initialized canonical execution state for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the current representative diagnostics
for `src/20000603-1.c` and `src/20030209-1.c`, confirm the prepared
`fpr:fs1` frame-slot facts still reach RV64, and record the owning boundary
plus the next focused coverage or repair packet.

## Watchouts

- Leave prepared frame-layout production out of scope unless inspection proves
  the published facts are stale.
- Do not special-case filenames, `fs1`, diagnostic text, unsupported markers,
  allowlists, or expected outputs.
- Preserve GPR callee-saved frame behavior while inspecting the non-GPR path.

## Proof

Lifecycle activation only. Validation should check `git diff --check -- plan.md
todo.md` and synchronize local plan-review state to Step 1.
