# Current Packet

Status: Active
Source Idea Path: ideas/open/808_lir_phi_scalar_bit_not_xor_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish native authority for the scalar `xor` result

## Just Finished

- Step 1 complete: `pr68376-2.c` reproduces the checked failure
  `LirPhiIncoming.value: must identify a known current-function LirValueId`.
  In `src/codegen/lir/hir_to_lir/expr/misc.cpp:134`, scalar
  `UnaryOp::BitNot` creates `tmp` with `fresh_tmp(ctx)`; the scalar branch
  emits `LirBinOp{tmp, "xor", promoted_ty, promoted_val, "-1"}` at line
  159 and returns that ID-less display operand at line 161. Ternary lowering
  preserves it through `then_source`/`then_coerced` and
  `retain_same_type_source_authority` (`misc.cpp:260-288`) into
  `LirPhiIncoming` (`:290-295`), so there is no consumer-side loss.
- Step 2 coverage location: add an adjacent focused test after
  `test_floating_unary_minus_ternary_phi_incoming_authority` in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`. Its direct positive
  fixture is `return condition ? ~left : ~right;`, asserting one PHI, two
  scalar `LirBinOp` `xor` producers, valid incoming IDs, and producer-ID
  equality. Its malformed case removes one PHI-referenced `xor` producer
  result and expects the unchanged verifier to reject the now-invalid PHI
  authority.

## Suggested Next

- Execute Step 2: publish a checked native current-function `LirValueId` at
  the scalar `UnaryOp::BitNot` `xor` producer and add the focused positive
  plus malformed-authority coverage identified above.

## Watchouts

- Keep this route limited to scalar bit-not `xor`; vector and complex bit-not,
  floating or scalar unary-minus, postfix old-value, PHI/ternary consumer,
  and verifier contracts remain out of scope. The existing PHI verifier
  already rejects missing, unknown, foreign, and stale authority.

## Proof

- Trace-only Step 1 reproduction: `./build/c4cll
  tests/c/external/gcc_torture/src/pr68376-2.c` (expected failure at the
  existing PHI authority verifier). No `test_after.log` was required or
  created for this trace-only packet. Step 2 must use the supervisor-selected
  fresh build and focused same-family coverage; after acceptance, 806 still
  requires the 100% full baseline before 804 can resume.
