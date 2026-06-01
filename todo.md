# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Relocate one narrow owner-local publication family

## Just Finished

Completed Step 2 from `plan.md`: relocated the comparison-owned missing
branch/fused-compare publication helpers into the AArch64 comparison owner.
`lower_missing_conditional_branch_condition_publication`,
`lower_missing_fused_compare_operand_publication`, and
`lower_missing_fused_compare_operand_publications` now live in
`comparison.cpp`/`comparison.hpp`.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `todo.md`

Comparison consumes dispatch/prepared publication facts through explicit
`DispatchBranchFusionHooks` entries. The generic
`prepared_publication_source_producer_for_value`,
`prepared_source_producer_instruction`, and prepared value-home publication
authority remain dispatch-owned; `dispatch_publication.cpp` keeps only a thin
legacy wrapper for the existing three-argument conditional publication API.

## Suggested Next

Execute the next Step 2 relocation packet against a narrow memory-owned
publication branch, or have the supervisor decide whether the remaining
dispatch-publication helper surface is ready for route review before another
owner relocation.

## Watchouts

- Keep `emit_value_publication_to_register` in place until more owner-local
  helper branches are extracted.
- Do not move the prepared producer lookup helpers unless the packet creates a
  shared prepared-authority surface; they are still consumed across scalar,
  globals, select, comparison, and memory paths.
- `lower_missing_conditional_branch_condition_publication` still has a
  compatibility declaration in `dispatch_publication.hpp` because an existing
  backend dispatch test calls the old three-argument entrypoint directly. The
  wrapper delegates to comparison through the same hook surface used by
  dispatch.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log"
```

Result: passed. The build completed and CTest reported `100% tests passed, 0
tests failed out of 169`.

Proof log: `test_after.log`
