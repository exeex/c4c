# Prepared I16 Formal ABI Publication

Status: Closed
Type: Follow-up producer repair idea
Parent: `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`

## Goal

Publish usable prepared scalar integer ABI/home facts for direct-call `I16`
formals so RV64 object emission can consume already-prepared parameter homes
without reconstructing producer ABI policy.

## Why This Exists

While executing active idea 395, `src/20000403-1.c` moved from instruction
fragment investigation to a producer-side `unsupported_param_home` blocker.
The failing prepared shape is the `i16 %p.win` formal in both `seqgt` and
`seqgt2`: the formal is stack-homed in prepared BIR through regalloc spill-slot
objects, but the function parameter has no usable prepared scalar integer ABI
fact for the incoming argument register/home.

The expected repair point is
`src/backend/prealloc/legalize.cpp` `direct_bir_call_arg_abi_repair()`. That
repair path already covers `I1`, `I8`, `I32`, `I64`, and `Ptr`, but not `I16`.
Fixing this in RV64 `object_emission.cpp` would duplicate producer ABI policy
inside the consumer.

## In Scope

- Inspect the direct BIR call/formal ABI repair path for scalar integer width
  handling.
- Extend producer-side prepared ABI publication for `I16` formals in the same
  semantic family as existing `I1`, `I8`, `I32`, `I64`, and `Ptr` support.
- Preserve prepared-BIR/regalloc ownership of incoming scalar parameter homes.
- Prove `src/20000403-1.c` no longer reaches RV64 object emission with an
  ABI-less stack-homed `i16` formal.
- Add or update narrow prepared-BIR/regalloc coverage when a local test surface
  already exists for direct formal ABI repair.

## Out Of Scope

- Teaching RV64 `object_emission.cpp` to infer missing incoming argument
  registers from parameter order and type.
- Rewriting object-route parameter-home admission to accept ABI-less scalar
  stack homes.
- Broad ABI redesign beyond the missing `I16` scalar integer-width case.
- Expectation rewrites, unsupported marker changes, or allowlist filtering.
- Treating unrelated instruction-fragment lowering failures as part of this
  producer repair.

## Acceptance

- `src/backend/prealloc/legalize.cpp` publishes prepared ABI/home facts for
  direct-call `I16` formals through the same producer-side mechanism used for
  adjacent scalar integer widths.
- `src/20000403-1.c` no longer fails with
  `unsupported_param_home: RV64 object route requires all parameters in
  supported GPR or prepared FPR register homes` because of an ABI-less
  stack-homed `i16 %p.win` formal.
- Prepared dumps or tests show the `seqgt` and `seqgt2` `i16 %p.win` formals
  have usable scalar integer ABI facts before RV64 object emission.
- The object emitter remains a consumer of prepared facts and does not gain a
  duplicate scalar formal ABI classifier.
- Narrow build/proof plus supervisor-selected backend proof are recorded in
  canonical `todo.md` / `test_after.log` by the executor.

## Closure Notes

- 2026-06-26: Closed after commit `f847dd20` added `I16` handling to
  `direct_bir_call_arg_abi_repair()`.
- `src/20000403-1.c` no longer fails with the producer-side ABI-less
  `i16 %p.win` `unsupported_param_home` blocker.
- Fresh probing now reaches the distinct RV64 object-route blocker
  `unsupported_scalar_compare_trunc`; continue that work under
  `ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md`.
- Close gate passed using canonical backend regression logs in non-decreasing
  mode: 326 passed before, 326 passed after, no new failing tests.

## Reviewer Reject Signals

- Reject fixes that special-case `src/20000403-1.c`, `seqgt`, `seqgt2`, or
  `%p.win` instead of repairing reusable `I16` formal ABI publication.
- Reject RV64 `object_emission.cpp` changes that infer the missing incoming
  register or stack home from formal order/type; that reconstructs producer ABI
  policy in the consumer.
- Reject expectation downgrades, unsupported marker changes, or allowlist
  filtering claimed as progress on this idea.
- Reject helper renames, diagnostic wording changes, or classification-only
  updates that leave `I16` direct formals without usable prepared ABI/home
  facts.
- Reject broad ABI rewrites that change unrelated scalar widths, aggregate
  passing, variadic handling, or object-route admission without local proof and
  explicit plan scope.
- Reject a green `src/20000403-1.c` result if prepared dumps still show the
  same ABI-less `i16` stack-home shape hidden behind a new abstraction name.
