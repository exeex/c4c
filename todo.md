# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 12
Current Step Title: Implement and prove the Step 11 selection

## Just Finished

Step 12 implementation now requires `LirNativeVectorAuthority` on the sole
direct vector-value `IndexExpr` `LirExtractElementOp` producer and requires its
native `i32` index type. The row-local verifier compares index kind and native
authority rather than index display text, while retaining current-function
result/vector IDs and structured lane/element shape coherence. Nearby coverage
now accepts native SSA and immediate/coerced `i32` indices and rejects missing,
invalid, mismatched, or undefined result/vector/index facts; missing/bad
shapes; and non-`i32` index types. `LirInsertElementOp` and accepted
ShuffleVector behavior were left unchanged.

## Suggested Next

Supervisor checkpoint: inspect the bounded diff and decide whether to accept
the required ExtractElement carrier seam before broader validation.

## Watchouts

The LIR schema has no per-ExtractElement requirement bit, but repository search
found exactly one producer, the selected direct vector IndexExpr lowering; the
verifier therefore requires the carrier at that sole operation seam. Do not
generalize to InsertElement or alter accepted zero-initializer ShuffleVector.

## Proof

Executor proof passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'` (6/6 backend tests). Supervisor-accepted proof: fresh backend before/after 6/6, monotonic guard PASS with documented `--allow-non-decreasing-passed`, and full CTest 3038/3038 passing. The supervisor owns the canonical regression logs.
