Status: Active
Source Idea Path: ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Or Narrow The Stack-Destination Authority Path

# Current Packet

## Just Finished

Completed Step 2 focused backend coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`.

Added
`rejects_ambiguous_non_parallel_multi_source_stack_destination_move_bundle`,
which builds a non-select `BeforeInstruction` binary-compare fixture with:

- two distinct register source homes
- both moves targeting the same stack-homed compare result
- `authority=none`
- no prepared parallel-copy bundle

The test verifies the shape remains fail-closed with
`AmbiguousNonParallelMultiSourceStackDestination` and preserves the shared
diagnostic:

```text
prepared move-bundle classifier rejected ambiguous non-parallel multi-source stack-destination authority
```

## Suggested Next

Step 3 should decide and implement the repair surface without weakening the new
fail-closed test: either stop producer-side publication of binary/compare
multi-source writes to one stack result slot, or improve the diagnostic path
around the existing classifier rejection.

## Watchouts

- The 574 FP binary acceptance criteria are complete; do not fold this
  move-bundle authority work back into FP binary lowering.
- Do not bypass the prepared move-bundle classifier broadly. The representative
  first blocker is currently a genuine ambiguity in prepared authority, not a
  valid ordered stack-destination move sequence.
- Valid stack-destination multi-source materialization should stay tied to
  select or parallel-copy authority, not filename/function/value-name matching.
- Step 3 implementation/diagnostic surface is split between the classifier in
  `src/backend/prealloc/prepared_object_traversal.cpp` and RV64 diagnostic /
  materialization handling in
  `src/backend/mir/riscv/codegen/object_emission.cpp`; if the desired repair is
  producer-side, the prepared move-bundle producer for binary/compare
  consumers must stop publishing multi-source writes to one stack result slot.

## Proof

Delegated proof passed; log path `test_after.log`.

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1
```

Result: build succeeded; `backend_riscv_object_emission` passed.
