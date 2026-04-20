# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 8 / 10
# Current Packet

## Just Finished

Step 3 migrated the adjacent prepared `CastInst` lane in
`prepared_local_slot_render.cpp` onto a dedicated per-op helper, so the
covered `ZExt`/`SExt` carrier transitions now route through
`render_prepared_cast_inst_if_supported` instead of staying as inline
block-loop special casing next to the binary helper.

## Suggested Next

Keep Step 3 bounded by extracting the remaining inline scalar load/store
family surface out of the local `rendered_load_or_store` lambda only where it
becomes a real per-op selector helper contract, then reassess whether Step 3
is exhausted before widening into Step 4 call or terminator work.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new cast helper is intentionally narrow: it only owns the already-
  covered `ZExt` materialized-compare handoff and `SExt` `i8 -> i32` carrier
  transition, and it should stay a selector-style family helper rather than
  growing into another block-scale dispatcher.
- The remaining Step 3 friction is now mostly concentrated in the
  load/store lambda; only extract the next seam if it produces a reusable
  prepared-legality helper instead of cosmetic lambda flattening.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_' 2>&1 | tee
/workspaces/c4c/test_after.log`. The build completed successfully after this
Step 3 bounded cast-family helper slice. The final `^backend_` subset in
`test_after.log` matched the accepted `test_before.log` failure set exactly:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
