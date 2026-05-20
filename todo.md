Status: Active
Source Idea Path: ideas/open/316_aarch64_frame_slot_layout_consistency.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Frame Layout Divergence

# Current Packet

## Just Finished

Step 1 read-only localization inspected the current generated artifacts for
`c_testsuite_aarch64_backend_src_00216_c` / `test_correct_filling`, but did
not find a current frame-size/frame-slot layout divergence to assign to an
implementation owner.

Current generated facts:

- `test_correct_filling` prepared summary reports `frame_size=100`,
  `frame_alignment=8`, saved `x20` at `stack104`, and saved `x21` at
  `stack112`. The emitted prologue allocates `sub sp, sp, #144`, saves `x20`
  at `[sp, #104]`, `x21` at `[sp, #112]`, and `x30` at `[sp, #120]`, then
  restores those registers and deallocates `add sp, sp, #144`. Ordinary
  accessed stack homes include offsets `16`, `20`, `24`, `32`, `40`, `44`,
  `48`, `52`, `56`, `64`, `68`, `72`, `76`, `80`, `88`, `92`, and `96`; all
  observed accesses are within the allocated frame.
- `test_zero_init` prepared summary reports saved `x20` at `stack80` and saved
  `x21` at `stack88`; its prepared stack layout covers object slots from
  offset `0` through `64` plus a pointer spill at offset `72` size `8`. The
  emitted prologue allocates `sub sp, sp, #112`, saves `x20` at `[sp, #80]`,
  `x21` at `[sp, #88]`, and `x30` at `[sp, #96]`, then calls
  `test_correct_filling` with `&b.a = sp`, `&c.a = sp + 20`, and
  `&d.a = sp + 52`; those addresses are inside the allocated frame.
- The large-frame `foo` path currently reports `frame_size=1609`,
  `frame_alignment=8`, saved `x20` at `stack1616`, and saved `x21` at
  `stack1624`. The emitted prologue allocates `sub sp, sp, #1648`, saves
  `x20` at `[sp, #1616]`, `x21` at `[sp, #1624]`, and `x30` at `[sp, #1632]`;
  high local accesses through `[sp, #1608]` and save/restore offsets through
  `[sp, #1632]` are covered by the allocation.

Owner classification: blocked localization / stale source evidence. The
current artifacts do not expose a first frame-layout owner for this packet.
They show coherent prepared frame sizes, slot offsets, save slots, prologue
allocation, and accessed addresses for the named callee, its caller, and the
large-frame reference path.

## Suggested Next

Return to supervisor/plan-owner for a lifecycle decision on idea 316, or
provide a fresh failing artifact/proof if `00216.c` still fails for a current
frame-layout reason. Do not proceed to an implementation packet from the stale
`sub sp, sp, #48` evidence.

## Watchouts

Do not reopen idea 314's legal large-offset instruction spelling unless new
evidence shows spelling is again the first bad fact. Keep the unrelated
transient `review/326_stdarg_byval_route_review.md` untouched.

Do not use filename-specific, function-specific, literal-offset-specific, or
expected-output-only fixes. This plan owns the general AArch64 frame
size/frame-slot offset consistency contract.

This packet also did not identify scalar stack publication, f128 transport,
semantic admission, or unrelated runtime mismatch as the first owner: no
out-of-frame access was found in the current generated code inspected for the
requested functions.

## Proof

Read-only artifact inspection only, per packet. Inspected current assembly and
prepared BIR dumps for `test_correct_filling`, `test_zero_init`, and `foo`.
No broad build, CTest run, or `test_after.log` overwrite was performed.
