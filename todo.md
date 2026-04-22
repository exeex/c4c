# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Classify The Same-Module Aggregate String/Mixed-Aggregate Boundary
Plan Review Counter: 1 / 5
# Current Packet

## Just Finished

Plan step `2.2` is now classified to branch `2.2.2`: the first bad
`fa1`/`fa2` fact is upstream prepared byval-home publication/layout, not the
helper-fragment contract. Read-only inspection showed the bounded helper lane
in `prepared_local_slot_render.cpp` only consumes prepublished homes:
`select_prepared_bounded_same_module_helper_call_render_if_supported`
dispatches byval pointer args to
`append_prepared_small_byval_payload_move_into_register_if_supported`
([src/backend/mir/x86/codegen/prepared_local_slot_render.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/prepared_local_slot_render.cpp:4471)),
which for aligned payloads calls
`resolve_prepared_named_ptr_published_payload_frame_offset_if_supported`
([prepared_local_slot_render.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/prepared_local_slot_render.cpp:4730)).
That resolver only reads from local-slot offsets, `PreparedValueHome`
stack-slot offsets, or `PreparedStackLayout` object/frame-slot offsets
([prepared_local_slot_render.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/prepared_local_slot_render.cpp:4748),
[src/backend/prealloc/prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp:229),
[src/backend/prealloc/prealloc.hpp](/workspaces/c4c/src/backend/prealloc/prealloc.hpp:3380),
[src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1057)).
The generated `00204.c.s` confirms those published homes already overlap
before the helper call: `s9` writes byte 8 to `rsp+6176` and `s10` starts at
that same `rsp+6176`, `s10` byte 8 writes `rsp+6184` and `s11` starts at
`rsp+6184`, then `s11` and `s12`, `s12` and `s13` repeat the pattern
([build/c_testsuite_x86_backend/src/00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3310),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3313),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3337),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3343),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3373),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3376),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3409),
[00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3412)).
The helper then merely passes those same published addresses in registers to
`fa1` (`lea rdi, [rsp + 6160]`, `lea rsi, [rsp + 6168]`, ...,
`lea r9, [rsp + 6200]`)
([00204.c.s](/workspaces/c4c/build/c_testsuite_x86_backend/src/00204.c.s:3451)).

## Suggested Next

Take branch `2.2.2`: rehome the seam to the upstream prepared byval-home
publication/layout owner and inspect where the `fa1`/`fa2` byval stack homes
are assigned at 8-byte spacing despite larger aggregate widths. The next
executor packet should stay read/write on that upstream publication/layout
lane rather than retrying helper-fragment or helper-call renderer changes.

## Watchouts

- The helper-lane boundary test is green under the current canonical fragment
  contract; this packet found no evidence that the contract itself is the
  first bad fact.
- The decisive ownership point is that the helper consumer reads offsets from
  `PreparedModuleLocalSlotLayout`, `PreparedValueHome`, and
  `PreparedStackLayout`; it does not author new aggregate spacing for these
  `fa1`/`fa2` calls.
- The generated `00204.c.s` overlap is concrete and local: `rsp+6176`,
  `rsp+6184`, `rsp+6192`, and `rsp+6200` each act as both the tail of one
  aggregate copy and the base of the next aggregate argument before the call.
- Keep the next packet bounded to upstream byval-home owner/allocation logic.
  Do not widen into helper-fragment rewrites, `Return values:`, or `stdarg:`
  while this aggregate-string seam is still the first visible mismatch.

## Proof

Fresh focused proof for this step-2.2 classification packet:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with
  `[RUNTIME_MISMATCH]`; the first visible mismatch is still the same-module
  aggregate string/mixed-aggregate seam (`sAJ AJT JT4 T4g 4gy gyz`,
  `AJT JT4 T4g 4gt gtG tGH`, and the mixed aggregate numeric line), followed by
  corrupted `Return values:` and `stdarg` output
- Proof log path: `test_after.log`
