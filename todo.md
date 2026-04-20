# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Consolidate Shared Scalar Operand And Compare Selectors
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 2.2 extracted a reusable prepared same-module global scalar-memory
selection helper in `prepared_local_slot_render.cpp` so the main guard-chain
global load/store paths and the bounded same-module helper store path no longer
re-resolve prepared symbol memory operands and same-module global legality
checks independently. The packet rewired those covered routes to consume the
shared selector surface without widening into broader arithmetic family
migration or Step 3 dispatch work.

## Suggested Next

Continue Step 2.2 with the next shared selector seam after same-module global
memory selection, preferably by consolidating a real prepared compare-legality
or compare-setup helper that can be shared across the remaining bounded and
guarded scalar routes without widening into call move-bundle work or Step 3
per-op dispatch.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- The new helper is intentionally limited to same-module scalar global-memory
  resolution; it centralizes prepared symbol-memory rendering and global
  legality lookup, but it does not yet unify nearby compare setup or broader
  local-memory selector state.
- Keep nearby `current_i32_name`, `previous_i32_name`, and compare-materialized
  state transitions explicit until there is a genuinely reusable compare or
  register-state selector contract instead of more cosmetic rewiring.
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
same-module global scalar-memory selector extraction, and the `^backend_`
subset kept the same four failing tests already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
