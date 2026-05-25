Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Value Materialization Ownership

# Current Packet

## Just Finished

Step 2 - Extract Value Materialization Ownership moved
`lower_scalar_mul_with_distinct_rhs_scratch` out of `dispatch.cpp` and into
`dispatch_value_materialization.cpp`, with the owner surface declared in
`dispatch_value_materialization.hpp`.

`dispatch_prepared_block` still routes the fallback `Mul` branch to the owner
and pushes the returned instruction. The moved implementation preserves the
prepared-result register path, stack-home scratch/result store path,
`value_publication_may_read_register_index` operand-order conflict handling,
and scalar-register recording.

## Suggested Next

Supervisor review/commit this Step 2 extraction slice. Step 2 appears
exhausted for small value-materialization extractions; suggested next lifecycle
direction is Step 3 publication/producer lookup.

## Watchouts

No call-source files, tests, or unrelated address-materialization/publication
helpers changed. Remaining dispatch-local address-materialization,
missing-compare publication, and same-block producer lookup responsibilities
belong to later publication/producer slices.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records' --output-on-failure > test_after.log 2>&1
```
