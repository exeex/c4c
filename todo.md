# Current Packet

Status: Active
Source Idea Path: ideas/open/815_lir_shuffle_vector_native_mask_lane_coherence_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair and cover bounded mask-lane coherence

## Just Finished

Step 2 repair completed: both existing scalar-to-vector splat constructors in
`src/codegen/lir/hir_to_lir/expr/binary.cpp` now publish one structured
`{Selected, 0}` lane per output lane and an explicit native
`ZeroInitializer` token, while retaining the `zeroinitializer` display mirror.
The verifier now demands that exact selected-zero contract. Nearby
`backend_lir_native_vector_authority` coverage accepts the valid carrier and
rejects missing lanes, bad count/type mirror, unstructured mask token,
inactive lanes, nonzero selected payloads, and invalid lane kinds. The actual
`scal-to-vec1.c` LLVM route reaches emission and retains its zero-initializer
shuffle display.

## Suggested Next

Supervisor-owned Step 3: perform the blocker-handoff proof and return
decision, including the required fresh full-baseline acceptance before any
parent reactivation.

## Watchouts

`zeroinitializer` is the display mirror for a vector of zero selection
indices; it must also retain native `ZeroInitializer` authority. This remains
a structured carrier repair only: no text recovery or shuffle-row semantics.

## Proof

Passed: `cmake --build --preset default`; representative route:
`./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/scal-to-vec1.c
-o /tmp/scal-to-vec1.ll`; and delegated matching proof:
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
2>&1` (6/6 passed). Proof log: `test_after.log`.
