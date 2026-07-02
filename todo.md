Status: Active
Source Idea Path: ideas/open/522_bir_route1_scalar_producer_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Apply Minimal Route1 Body Extraction Or Preserve Decision

# Current Packet

## Just Finished

Step 3 - Apply Minimal Route1 Body Extraction Or Preserve Decision completed.

Moved the selected route1 scalar producer body definitions from
`src/backend/bir/bir.cpp` into new `src/backend/bir/bir_route1.cpp`:
`route1_source_value_identity`, `route1_immediate_integer_constant`,
`route1_producer_kind`, `route1_produced_value`,
`route1_producer_instruction_identity`, `route1_producer_record`,
`route1_build_producer_index`, `route1_find_same_block_scalar_producer`,
`route1_find_materialization_availability`, and both
`route1_evaluate_same_block_integer_constant` wrappers.

Moved only the selected private route1-only helpers into the new TU:
`produced_value_for_route1_producer` and
`route1_evaluate_same_block_integer_constant_impl`.

Preserved public declarations in `src/backend/bir/bir.hpp`; no route
declarations, route records, route semantics, tests, expectations, unsupported
markers, downstream route bodies, or comparison-only helpers were changed.

Updated `tests/backend/bir/CMakeLists.txt` so the direct-source
`backend_lir_to_bir_notes_test` target also compiles
`src/backend/bir/bir_route1.cpp`.

## Suggested Next

Execute the next plan packet for post-move bookkeeping and acceptance review:
record the accepted Step 3 proof, decide whether a reviewer pass is needed, and
roll validation state forward according to the supervisor workflow.

## Watchouts

`bir_route1.cpp` deliberately includes only `bir.hpp`; the selected route1
bodies did not require `bir_private.hpp`. Comparison-only helpers remain in
`bir.cpp`: `produced_value_for_comparison_producer`,
`comparison_producer_kind_for_inst`, `is_comparison_binary_opcode`,
`find_unique_comparison_producer`, and
`evaluate_comparison_integer_constant`.

## Proof

Validation run:
- `git diff --check`
- `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: build passed and backend subset passed.

Proof log:
- `test_after.log`
