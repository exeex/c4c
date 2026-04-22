# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Repair The Same-Module Aggregate String/Mixed-Aggregate Call-Lane Seam
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Plan step `2.1` advanced inside the owned x86 consumer path in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`.
Prepared same-module and direct-extern call rendering now keep the full-reserve
stack-source fix for stack arguments, but emit the reserve in two steps before
materialization: the stack-arg area first, then the 8-byte call-alignment pad.
That keeps the HFA long-double `Arguments:` repair while restoring the helper-lane
asm contract expected by `backend_x86_handoff_boundary`.
Fresh proof now has `backend_x86_handoff_boundary` green again, and the first
visible `00204.c` mismatch stays past the repaired HFA long-double seam:
`31.1`, `32.1 32.1`, `33.1 33.2 33.3`, and `34.1 34.2 34.3 34.4` all print
correctly before the later remaining corruption.

## Suggested Next

Take the next packet on plan step `2.2` inside the owned x86 consumer path.
Start from the new first bad facts in `00204.c`: the same-module aggregate
string/mixed-aggregate lines after the repaired HFA long-double block
(`sAJ AJT JT4 T4g 4gy gyz`, `AJT JT4 T4g 4gt gtG tGH`, and
`0 14.1 0.0 12 24.1 24.4 ... nan 34.1`). Only treat the still-corrupt
`Return values:` and empty `stdarg:` sections as the next seam after those
aggregate-line facts are either repaired or explicitly rehomed.

## Watchouts

- Keep the new source-bias contract tied to the full reserved outgoing call
  stack, not just `stack_arg_bytes`; that is what fixed the HFA long-double
  seam.
- The helper-route regression came from collapsing the stack-arg reserve and
  call-alignment pad into one `sub rsp, 24`. The accepted shape now keeps the
  semantic fix while restoring the earlier two-step reserve pattern.
- Do not reopen the importer seam unless the prepared `stdarg` handoff loses
  its preserved `ptr byval(size=..., align=...)` metadata again.
- The remaining runtime frontier is later than the repaired HFA long-double
  `Arguments:` block and is still inside the owned x86 consumer path.
- Do not skip straight to `Return values:` or `stdarg:` debugging while the
  aggregate-string/mixed-aggregate lines are still the first bad visible facts.

## Proof

Fresh focused proof for this step-2.1 x86 consumer packet:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with
  `[RUNTIME_MISMATCH]`, but the first visible mismatch stays past the repaired
  HFA long-double `Arguments:` seam. The next bad facts are the later
  same-module aggregate string/mixed-aggregate lines, followed by corrupted
  `Return values:` and `stdarg` output
- Proof log path: `test_after.log`
