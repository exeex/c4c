# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Consolidate Shared Scalar Operand And Compare Selectors
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 2.2 extracted a reusable prepared named-vs-immediate `i32` compare
selector plus a shared `eax` compare-setup renderer in
`prepared_local_slot_render.cpp`, and rewired the materialized compare path
plus the raw and prepared immediate-branch planners to consume that shared
compare surface instead of open-coding the same immediate-selection and
`test`/`cmp` setup logic independently.

## Suggested Next

Continue Step 2.2 by consolidating the next compare-planning seam around
candidate compared-value resolution in the prepared local guard and bounded
guard-chain routes, so nearby callers can consume one shared compare-plan
surface before Step 2.3 call-lane legality work begins.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new compare helper is intentionally limited to named-vs-immediate `i32`
  compare planning and shared `eax` setup emission; it does not yet unify
  candidate compared-value discovery across the nearby guarded local and
  compare-driven branch routes.
- Keep nearby `current_i32_name`, `previous_i32_name`, and compare-materialized
  state transitions explicit until there is a genuinely reusable compared-value
  or register-state selector contract instead of cosmetic rewiring.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran the proof command `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'` and captured the output in
`test_after.log`. The build completed successfully after this Step 2.2
compare-selector extraction, and the `^backend_` subset kept the same four
failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
