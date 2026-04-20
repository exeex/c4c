# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Covered Terminator And Call Families
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 3.4 audited `prepared_local_slot_render.cpp` for any remaining real
covered scalar instruction-family seam and concluded Step 3 is structurally
exhausted. The active prepared single-block route already sends supported
scalar load/store, binary, and cast families through the named per-op helper
surfaces in this file, and the remaining inline work in that route is the call
lane plus branch/return terminator handling that belongs to Step 4.

## Suggested Next

Execute Step 4 with one bounded call or terminator migration packet in the
prepared single-block route. The strongest next seam is the inline same-module
helper/direct-call lane inside `render_block`, keeping the packet limited to
prepared per-call selection rather than reopening scalar instruction-family
work.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- Step 3 is closed out by audit, so follow-up should stay on Step 4 call or
  terminator migration instead of forcing another scalar helper extraction.
- Keep Step 4 focused on prepared per-call or per-terminator selection in the
  active route, not on widening legacy whole-function fallback families.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_' 2>&1 | tee
/workspaces/c4c/test_after.log`. The build completed successfully after the
Step 3.4 audit. The final `^backend_` subset in `test_after.log` matched the
accepted `test_before.log` failure set exactly, with no new failures:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
