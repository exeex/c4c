# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Covered Scalar Instruction Families
Plan Review Counter: 7 / 10
# Current Packet

## Just Finished

Step 3 migrated the inline prepared `BinaryInst` lane in
`prepared_local_slot_render.cpp` onto a dedicated per-op helper, so the
covered `i32` add/sub-immediate and materialized `eq`/`ne` compare family now
routes through `render_prepared_i32_binary_inst_if_supported` instead of
staying mixed into the block render loop.

## Suggested Next

Keep Step 3 bounded by moving one adjacent scalar family that still has
route-local emission, preferably the remaining cast lane in the same block
loop so `ZExt`/`SExt` carrier transitions stop depending on inline
stateful control flow before widening into call or terminator work.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new binary helper is still intentionally narrow: it only owns the
  already-covered `i32` add/sub-immediate and materialized `eq`/`ne` compare
  path, and it should stay a selector-style family helper rather than becoming
  a second block-scale dispatcher.
- The adjacent cast lane still depends on `current_i8_name` and
  `current_materialized_compare`; keep the next packet focused on that carrier
  transition seam instead of mixing it with scalar-call or branch work.
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
Step 3 bounded binary-family helper slice. The final `^backend_`
subset in `test_after.log` matched the accepted `test_before.log` failure set
exactly:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
