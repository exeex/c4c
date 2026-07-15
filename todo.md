# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Audit and select the remaining InsertElement authority row

## Just Finished

Step 12 is accepted in `d491013e9`. Its implementation requires `LirNativeVectorAuthority` on the sole
direct vector-value `IndexExpr` `LirExtractElementOp` producer and requires its
native `i32` index type. The row-local verifier compares index kind and native
authority rather than index display text, while retaining current-function
result/vector IDs and structured lane/element shape coherence. Nearby coverage
now accepts native SSA and immediate/coerced `i32` indices and rejects missing,
invalid, mismatched, or undefined result/vector/index facts; missing/bad
shapes; and non-`i32` index types. `LirInsertElementOp` and accepted
ShuffleVector behavior were left unchanged.

## Suggested Next

Audit only the remaining `LirInsertElementOp` producer seam against the
accepted 811 vector carrier and the accepted ShuffleVector precursor. Record
one concrete row-local contract plus its nearby positive/malformed matrix.
Do not reuse the previously rejected scalar-to-vector selection without a
fresh audit, generalize ExtractElement, or change accepted ShuffleVector.

## Watchouts

The accepted ExtractElement seam has no per-row requirement bit because its
sole producer is direct vector IndexExpr lowering. InsertElement must receive
its own fresh authority/seam decision; do not infer it from the existing
ShuffleVector precursor or from display text.

## Proof

Executor proof passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'` (6/6 backend tests). Supervisor-accepted proof: fresh backend before/after 6/6, monotonic guard PASS with documented `--allow-non-decreasing-passed`, and full CTest 3038/3038 passing. The supervisor owns the canonical regression logs.
