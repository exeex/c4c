# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Audit and select one remaining vector authority row

## Just Finished

Step 7 reassessed the source after Step 6 completed the selected terminal direct-complex `LirInsertValueOp`
producer in `StmtEmitter::emit_complex_binary_arith`'s
`require_direct_aggregate_ssa` branch.  The verifier now treats this selected
native result as current-function owned and, without parsing display text,
requires its existing aggregate result type to provide an ordered native
layout, its index to select a field, and its element type to match that field.
The existing result-ID/aggregate-result-type handoff to the immediate
`LirExtractValueOp` remains a compatibility consumer.

Nearby lowering-based coverage retains one valid terminal insert and rejects
missing, invalid/unknown, and foreign result authority; missing/conflicting
aggregate result type; negative/out-of-range index; and element-field type
conflict.  A stale producer/extract display pair with the same valid ID still
verifies, demonstrating that display does not select or repair authority.

The accepted Step 6 implementation is `8fe6c3569`; Step 5's row-selection
audit is `270c6a93e`. Focused frontend binary proof and `^backend_` matching
before/after guard passed 5/5, and the supervisor accepted the hook-generated
full baseline at 3037/3037.

## Suggested Next

Execute Step 8 only: inspect `LirInsertElementOp`, `LirExtractElementOp`, and
`LirShuffleVectorOp` producer/use seams; select exactly one row only if its
current-function result/use identity and row-local vector type/index/mask facts
can be stated from existing structured carriers. Record the selected seam and
positive/malformed matrix in this file before implementation, leaving the
other two rows unselected. If all candidate routes require generic vector
layout/provenance/mask publication, stop without code changes and request a
separate blocker with the Step 9 return point.

## Watchouts

Do not widen the completed insert row into generic binop/extract provenance or
generic vector layout. `LirInsertElementOp`, `LirExtractElementOp`, and
`LirShuffleVectorOp` remain unselected and unchanged. Do not recover vector
facts from display text; any generic vector layout, provenance, or mask
publication prerequisite is a separate blocker.

## Proof

Step 8 is an audit-only packet. Preserve the accepted Step 6 proof: fresh
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'` passed 5/5 backend tests; nearby
`./build/tests/frontend/frontend_lir_call_type_ref_test` passed; matching
`^backend_` before/after guard passed 5/5; and the supervisor accepted the
hook-generated full baseline at 3037/3037. Do not modify canonical root
`test_before.log` or `test_after.log` for this audit.
