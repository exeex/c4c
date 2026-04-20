# Execution State

Status: Active
Source Idea Path: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Prepared Dispatch Surface
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 1 kept the prepared x86 route structural by adding
`PreparedX86FunctionDispatchContext`-based wrappers for the local i32 and i16
guard helpers, then consolidating the module entry fallback chain behind one
`render_local_structural_dispatch_if_supported` helper so the prepared module
entry path now dispatches through packaged function context instead of
repeating long raw-parameter wiring for each guarded family.

## Suggested Next

Keep Step 1 structural and continue moving remaining prepared x86 entry lanes
onto helper-style function or block dispatch boundaries, especially any paths
that still reach legacy guard or countdown renderers through repeated raw
parameter lists.

## Watchouts

- Do not add new whole-function or named-testcase x86 matcher families.
- Consume prepared CFG, value-home, move-bundle, and addressing facts instead
  of re-deriving them from local emitter shape.
- Keep idea 59 focused on scalar instruction selection structure, not on
  reopening upstream ownership from ideas 58, 60, or 61.
- Prefer one coherent instruction-family migration per packet over broad
  emitter rewrites.
- `render_prepared_countdown_entry_routes_if_supported` is still wired from
  `emit_prepared_module` through a raw parameter list, so it is a natural
  candidate for the next Step 1 structural dispatch extraction.
- The matching `^backend_` before/after logs are not fully green: both
  `test_before.log` and `test_after.log` fail in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Supervisor-side regression evidence now uses matching before/after runs of
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`, with results captured in
`test_before.log` and `test_after.log`. This packet reran that exact command
into `test_after.log`; both logs show the same four failing tests:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
--before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported `result: PASS`, so the delegated backend subset is non-regressive for
this Step 1 structural slice even though the subset is not fully green.
