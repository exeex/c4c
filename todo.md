# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Overlapping Call-Lane Source-Preservation Seam
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Plan step `2` advanced through the active same-module local small-byval caller
seam in `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`.
This packet changed the bounded-helper consumer so helper-shaped same-module
calls with a prepared `call_arg_stack_to_register` bundle now fall through to
the generic same-module renderer, and it changed the small-byval payload loader
to prefer the prepared stack object offset for that payload instead of the
local-slot lane base. As a result, the earliest `arg()` calls now consume the
repaired publication correctly:
`mov BYTE PTR [rsp + 96], al`, `lea rdi, [rsp + 96]`,
`movzx edi, BYTE PTR [rdi]`, `call fa_s1`, and similarly for `fa_s2` / `fa_s3`.
This packet then repaired the later mixed-aggregate caller overlap too by
publishing local aggregate slice stores back into their aggregate-root homes
and by making the small-byval payload handoff prefer those synchronized homes.
The `fa4(...)` caller no longer overlaps `s1`, `s2`, and `s3` with the later
`hfa14` scratch; the rebuilt asm now uses root-home handoff like
`mov BYTE PTR [rsp + 6264], al`, `lea rdi, [rsp + 6264]`,
`mov BYTE PTR [rsp + 6280], al`, and `lea rdx, [rsp + 6280]`.
`backend_x86_handoff_boundary` remains green. `00204.c` still fails, but the
runtime crash has moved past the caller seam and into the `fa4` callee-side
mixed aggregate unpack path.

## Suggested Next

Stay in plan step `2`, but move to the callee-side mixed aggregate unpack seam
in the same renderer file. The next slice should repair the same-module callee
prologue for `fa4(s1, hfa14, s2, hfa24, s3, hfa34)` so mixed byval argument
spill/unpack does not use overlapping slots like `[rsp]` with `[rsp + 4]` and
`[rsp + 20]` with `[rsp + 22]`.

## Watchouts

- The helper-prefix contract remains a guardrail and is still green:
  `backend_x86_handoff_boundary` still finds the canonical helper fragments
  `mov BYTE PTR [rsp], al` plus `lea rdi, [rsp]` for the dedicated helper test.
- The active `00204.c` route now reaches the generic same-module small-byval
  renderer for the earliest calls, and the rebuilt asm shows the corrected
  payload bases (`[rsp + 96]`, `[rsp + 97]`, `[rsp + 99]`, ...).
- The old `fa4` caller overlap is repaired. The rebuilt caller asm now reloads
  `s1`, `s2`, and `s3` from root-home offsets `6264`, `6280`, and `6288`
  instead of the overlapped slice range `364..369`.
- The next blocker is callee-side, not caller-side. `fa4` still crashes in
  `printf`, and its prologue now stands out as the next bad seam:
  `mov QWORD PTR [rsp], rdi` followed by `mov QWORD PTR [rsp + 4], rsi`,
  and later `mov QWORD PTR [rsp + 20], rdx` followed by
  `mov QWORD PTR [rsp + 22], r8`.
- `gdb -batch -ex run -ex bt --args ./build/c_testsuite_x86_backend/src/00204.c.bin`
  still lands in `fa4` / `printf`, which is consistent with callee-side
  mixed-byval unpack corruption after the caller handoff was repaired.

## Proof

Fresh focused proof for this renderer follow-up packet:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with
  `[RUNTIME_NONZERO] exit=Segmentation fault`; the same-module caller-side
  small-byval and mixed-aggregate scratch seams are repaired, but `fa4` still
  crashes in the callee-side mixed aggregate unpack path
- Proof log path: `test_after.log`
