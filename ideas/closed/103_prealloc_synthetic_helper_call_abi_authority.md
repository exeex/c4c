# Prealloc Synthetic Helper Call ABI Authority

## Goal

Document or model the ABI authority for synthetic i128 and f128 runtime helper
calls that are planned in prealloc without corresponding source BIR `CallInst`
ABI carriers.

## Why This Exists

The call ABI authority audit found that `regalloc/runtime_helpers.cpp` selects
libgcc and soft-float helper callees for i128 and f128 operations from BIR
opcode/type facts, then `i128_runtime_helpers.cpp` and
`f128_runtime_helpers.cpp` synthesize helper arg/result ABI bindings, carrier
population, and boundary policy. This is intentional prealloc runtime
legalization for helper calls that do not exist as source BIR calls.

The unresolved contract is whether prepared-only helper call ABI remains the
durable authority, or whether helper calls should gain explicit BIR-like helper
call carriers. F128 comparison helpers also bridge an `I32` helper result into
the BIR `I1` comparison result, which should be an explicit semantic contract.

## In Scope

- Audit i128 and f128 helper call ABI binding, helper callee synthesis, and
  helper-result ownership semantics.
- Decide whether helper ABI facts remain prepared-only or should be surfaced
  through explicit BIR-like helper-call metadata.
- Document or implement the F128 comparison helper-result-to-BIR-result bridge
  as a named contract.
- Add proof for representative i128 helper, f128 arithmetic/cast helper, and
  f128 comparison helper routes.

## Out Of Scope

- Treating synthetic helper callee selection as duplicate source-call direct
  callee authority.
- Moving physical clobber, preservation, carrier marshaling, or register/stack
  movement out of prealloc.
- Rewriting unrelated special-carrier analysis.
- Changing helper names or expectations without capability proof.

## Acceptance Criteria

- Synthetic helper call ABI authority is explicitly classified as prepared-only
  or represented through a structured helper-call carrier.
- Helper arg/result ABI binding and F128 comparison result bridging have a
  named contract that reviewers can verify.
- Proof covers i128 helper ABI binding, f128 helper ABI binding, and the F128
  comparison result bridge.

## Reviewer Reject Signals

- The change claims duplicated BIR direct-callee cleanup for helper calls even
  though no source BIR `CallInst` exists.
- The route only renames helper structures while leaving the helper ABI
  authority and F128 result bridge implicit.
- Clobber, preservation, or carrier movement is moved into BIR without a
  proven semantic need.
- Tests are weakened, marked unsupported, or narrowed to one helper name while
  nearby helper families remain unexamined.
- The implementation broadens into unrelated runtime-helper or regalloc
  rewrites.

## Close Note

Closed on 2026-06-03.

The source idea is complete. Synthetic i128 and f128 runtime helpers remain
prepared-only runtime legalization artifacts rather than source BIR direct
calls. Prepared helper records are now the reviewable authority surface for
semantic helper ABI facts, including helper family/kind, source opcode or cast,
source/result types and widths, callee identity, result ownership, ABI
transition, argument/result banks/counts/lanes, value identities, and
selected-call ownership.

Physical call planning remains prealloc/MIR authority: carrier lanes,
full-width/scalar homes, ABI register placement, marshaling moves,
caller-saved clobbers, live preservation, stack/register placement, and
AArch64 materialized registers. AArch64 consumers fail closed without valid
prepared helper authority and do not reconstruct synthetic helper ABI from
source BIR direct-call metadata.

The f128 comparison helper-result bridge is named and covered by the
`prepared_f128_runtime_helper_has_scalar_cmp_result_bridge_contract` proof
surface, including helper `I32` result ownership, BIR `I1` ownership, zero-test
behavior, `consumes_helper_cmp_result`, `owns_bir_i1_result`, and AArch64 `I1`
materialization.

Proof status: final backend validation passed with `169/169` backend tests, and
the close-time backend regression guard passed with `169/169` before and after,
no new failures, and no resolved failures. Coverage includes i128 helper ABI
binding, f128 arithmetic/cast helper ABI binding, and f128 comparison result
bridging.
