# Current Packet

Status: Active
Source Idea Path: ideas/open/519_rv64_object_emission_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Materialize Follow-Up Ideas

## Just Finished

Step 4 - Materialize Follow-Up Ideas is complete. Created concrete
behavior-preserving follow-up source ideas from
`docs/rv64_object_emission_cleanup/staged_followups.md`:

- `ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md`
- `ideas/open/535_rv64_object_frame_stack_helper_cleanup.md`
- `ideas/open/536_rv64_object_prepared_module_admission_shell_cleanup.md`
- `ideas/open/537_rv64_object_local_memory_helper_cleanup.md`
- `ideas/open/538_rv64_object_global_address_helper_cleanup.md`
- `ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md`
- `ideas/open/540_rv64_object_select_edge_publication_helper_cleanup.md`
- `ideas/open/541_rv64_object_call_variadic_return_fragment_cleanup.md`
- `ideas/open/542_rv64_object_function_traversal_facade_cleanup.md`
- `ideas/open/543_rv64_object_data_symbol_fixup_module_cleanup.md`

Updated the staged follow-up artifact with a materialized idea mapping. Each
idea includes goal, why it exists, in-scope work, out-of-scope work,
acceptance criteria, and concrete reviewer reject signals. The ideas keep RV64
capability repair, gcc_torture expectation changes, unsupported marker changes,
target-side inference, and implementation edits out of scope.

## Suggested Next

Execute Step 5 from `plan.md`: close readiness review for idea 519. Verify the
durable artifact contains the Step 1 baseline, Step 2 comparison, Step 3 staged
follow-up list, Step 4 materialized idea links, and no implementation changes.

## Watchouts

- This is an analysis umbrella; do not move RV64 implementation code in this
  plan.
- Keep F128/gcc_torture capability repair, expectation changes, unsupported
  marker changes, and target-side inference out of the materialized ideas.
- Treat RV64 `calls.cpp`, `memory.cpp`, `globals.cpp`, `returns.cpp`,
  `prologue.cpp`, and `variadic.cpp` as historical layout references unless a
  materialized idea explicitly creates or proves live compiled ownership.
- Keep `prepared_function_to_object_function`,
  `fragment_for_prepared_instruction`, symbol/fixup module assembly, and
  prepared data-object emission as late or central boundaries.
- Do not materialize one broad catch-all cleanup idea that hides monolithic
  coupling behind new filenames.

## Proof

Lifecycle/docs-only source-idea materialization; no build run per delegated
proof. Evidence sources are recorded in
`docs/rv64_object_emission_cleanup/staged_followups.md`, including the
materialized idea mapping. No `test_after.log` was produced or rewritten.
