# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 11
Current Step Title: Audit and select one remaining vector authority row

## Just Finished

Step 11 audit selected exactly `LirExtractElementOp`, limited to the direct
vector-value `IndexExpr` lowering in `src/codegen/lir/hir_to_lir/expr/misc.cpp`
(the native `fresh_value` result, `emit_rval_operand` vector/index uses,
coerced `i32` index, vector shape, and `LirNativeVectorAuthority` construction
at lines 367--388). Its verifier seam is the existing per-function
`verify_vector_authority(*op, "LirExtractElementOp", ...)` path in
`src/codegen/lir/verify.cpp` (lines 2662--2720): it checks the current-function
result and vector-use IDs, native lane/element shape mirrors, and the exact
structured index operand/type without display-text recovery.

Required positive matrix for Step 12: (1) a direct vector `IndexExpr` whose
vector and `i32` index are native current-function values; (2) the same route
with an immediate/coerced `i32` index; in both forms the result ID, vector-use
ID, native index value/type, and `<lanes x element>` shape must agree.
Required malformed matrix: missing/invalid/foreign/undefined result ID;
missing/foreign/undefined vector-use ID; missing native shape, zero lanes, or
shape/display element mismatch; missing native index, index operand mismatch,
undefined index SSA ID, or non-`i32` index type; and a misleading display
mirror for any of those facts. These are row-local verifier cases, not
rendered-LLVM or testcase-name probes.

Explicitly excluded `LirInsertElementOp`: its only current lowering sites are
the scalar-to-vector poison seed in `expr/binary.cpp` (lines 289--298 and
410--416), which is the former rejected Step 9 InsertElement selection and is
now only the accepted ShuffleVector precursor. Re-selecting it would violate
the plan's no-reuse instruction; no distinct InsertElement native-carrier seam
was found in the bounded audit.

## Suggested Next

Implement only the selected direct vector `IndexExpr` `LirExtractElementOp`
contract: make its native carrier required for that route and add the listed
row-local result/vector/index/shape coherence checks and nearby positive plus
malformed coverage. Leave InsertElement unchanged.

## Watchouts

Steps 1--10 remain accepted. This selection relies on 811/814 only for their
accepted reusable carrier handoff, not as ExtractElement proof. Do not recover
facts from display text, generalize the accepted zero-initializer shuffle
splat, or reuse the rejected scalar-to-vector InsertElement route. If the
direct IndexExpr path cannot require the recorded carrier without widening
into generic provenance, stop for a separate blocker and return to Step 11.

## Proof

Audit-only packet: no build or test was required or run. Step 12 needs a fresh
build, nearby same-feature positive/malformed coverage, the matching backend
guard, and supervisor-selected full checkpoint; `test_after.log` is not
applicable to this audit-only packet.
