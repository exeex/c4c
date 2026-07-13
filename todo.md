# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid complex-valued direct, deep-pointer,
  and fixed multidimensional pointer-element globals through an explicit BIR
  `Complex` storage kind and typed integer/floating component facts.
- Exact component width, reconstructed storage spelling, pointer depth,
  dimensions, and object/initializer facts survive Foundation, Raw BIR, and
  Canonical BIR; complex long/ulong storage remains `i64` on I686 as required
  by the producer contract.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- Complex component semantics are typed facts, never inferred from LLVM
  spelling. Vector, function-pointer, pointer-to-array, reference, aggregate,
  va-list, unexpected-mirror, and malformed component neighbors remain closed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
