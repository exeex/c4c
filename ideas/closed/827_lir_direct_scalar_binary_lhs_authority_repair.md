# LIR DirectScalar Binary-LHS Authority Repair

Status: Open
Type: narrow native producer/verifier blocker
Blocked Parent: `ideas/open/825_lir_next_body_parameter_authority_handoff.md`, Step 2

## Goal

Repair only the native DirectScalar current-function parameter authority for
`LirBinOp.lhs`, so the structured producer and verifier agree when a binary
operation directly uses that parameter as its LHS.

## Why This Exists

The preserved, unaccepted Idea 825 Step 2 work advances the exact focused
frontend CTest past its earlier `LirSwitch.selector` abort, but it now stops
at `LirBinOp.scalar_lhs_parameter_authority: is required when LirBinOp.lhs
uses a native direct-scalar parameter`. Step 1 found that the producer requires
the native definition type to equal the binary operation type, while the
verifier's missing-authority relation can match the same LHS value and ABI
without that type predicate. The exact full-route operation and a standalone
reproduction of that mismatch must be identified before a repair is designed.
This remains a distinct binary-LHS producer/verifier first bad fact; closed
Idea 820's bounded `ull` publication does not own or authorize it.

## In Scope

- Trace the native binary-operation producer that creates a `LirBinOp` whose
  `lhs` is a current-function `DirectScalar` parameter, and the matching
  `verify_scalar_binary_lhs_authority` check in
  `src/codegen/lir/verify.cpp`.
- Before implementation, identify the first failing lowered operation from the
  full parent diagnostic route and create a minimal standalone reproduction of
  its value/ABI-match plus definition-type/operation-type mismatch.
- Publish/check exactly the existing `LirBinOp.scalar_lhs_parameter_authority`
  tuple: native parameter `LirValueId`, current-function `LinkNameId` owner,
  parameter index, `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`,
  `Lhs` role, and exact `LirBinOp.lhs`/operation-type coherence.
- Add focused nearby positive and malformed-authority coverage in
  `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`, then
  run a fresh build and that independently attributable CTest. This proof must
  not exercise or require the preserved Idea 825 selector slice.

## Out Of Scope

- Direct `LirSwitch.selector` authority, its `SwitchSelector` role, and all
  unaccepted Idea 825 code/test changes currently in the shared worktree.
- Raw-BIR/importer/builder work, generic parameter admission, ABI conversion,
  binary RHS, return-value, direct-pointer, or any second authority row.
- Reopening closed 820's `ull` publication, presentation-derived authority,
  test expectation weakening, or unrelated verifier redesign.

## Acceptance Criteria

1. The actual full-route binary-LHS failure and a standalone reproduction both
   establish the native DirectScalar value/ABI match and the differing
   definition/operation types before any producer repair is selected.
2. The native binary-LHS producer emits the exact structured authority only
   when its LHS directly matches one current-function DirectScalar parameter.
3. The verifier rejects missing, invalid, duplicate, foreign,
   owner/index/type/ABI/role-invalid, and LHS or operation-type-incoherent
   authority; unselected forms remain fail closed.
4. A fresh `cmake --build --preset default` and exact
   `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'`
   provide independently attributable focused acceptance evidence, including
   the nearby positive/malformed binary-LHS coverage, without crediting or
   requiring Idea 825's selector slice. The full
   `^frontend_lir_call_type_ref$` CTest remains a parent/composite checkpoint
   for Idea 825 after it resumes; it is not claimed as 827 evidence.
5. The handoff resumes Idea 825 exactly at Step 2 with its selector work still
   unaccepted and untouched by this blocker.

## Reviewer Reject Signals

- Reject a named-test bypass, expectation downgrade, or verifier exception
  that hides the same missing `scalar_lhs_parameter_authority` failure.
- Reject a helper rename, classification-only change, or display/type-string
  reconstruction claimed as native binary-LHS capability progress.
- Reject edits to direct switch-selector authority, the unaccepted Idea 825
  worktree slice, Raw-BIR/importer code, or any other parameter-use row.
- Reject acceptance without nearby positive and malformed coverage plus the
  exact independently attributable `^frontend_lir_function_signature_type_ref$`
  CTest, any use of the dirty selector slice to clear it, or any claim that
  the parent `^frontend_lir_call_type_ref$` checkpoint already passes or that
  closed 820 already owns this gap.
- Reject a producer predicate change or an arithmetic test shape (including
  `ull x + 1`) offered without the exact full-route failing operation and a
  standalone reproduction of the definition-type/operation-type mismatch.

## Resumption Record

- Last accepted progress: Step 1, "Trace the binary-LHS producer and authority
  seam," is accepted and committed as `26c000c01`. It established only the
  possible producer/verifier seam; it did not prove a failing binary operation.
- Interrupted step: Step 2, "Identify the actual failing operation and
  reproduce its type relation."
- Exact disproof: after a fresh successful `cmake --build --preset default`,
  the parent diagnostic command
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  aborts at `verify_scalar_binary_lhs_authority` on
  `lir_floating_unary_minus_ternary_phi_authority`. The first operation is
  `LirBinOp result %t6 value 4, opcode=fneg, type=double, lhs=%p.left value 1,
  rhs empty`; its matching native parameter is value 1, index 1, type `double`,
  owner `LinkNameId 1`, ABI `DirectScalar`. The types match. The missing
  publication is therefore produced by unary lowering in `expr/misc.cpp`, not
  by this idea's binary producer. A truthful standalone
  definition-type/operation-type mismatch reproduction cannot be made.
- Blocker and scope boundary: `ideas/open/828_lir_direct_scalar_unary_fneg_authority.md`
  owns the newly identified native DirectScalar unary floating `fneg`
  `LirBinOp` authority gap. It is outside this idea's binary-producer scope.
- Return disposition: after the unary blocker reports its result, return here
  only for an explicit repair/close decision. Do not reuse this unproven
  mismatch premise, and preserve the original binary-LHS scope. Step 2 made no
  code or test changes and produced no acceptance proof.

## Blocker Return Record

- Idea 828 returned with its unary `fneg` scope accepted: clean detached
  `524b24f64^` and clean detached `524b24f64`, each configured with
  `-DENABLE_C4C_BACKEND=ON`, built and directly ran `frontend_hir_tests`, each
  printing `PASS: frontend_hir_tests`.
- The preserved dirty aggregate still makes the CTest-form
  `^frontend_hir_tests$` probe SEGFAULT. The dirty selector-authority work in
  `core.cpp`, `ir.hpp`, `verify.cpp`, and
  `frontend_lir_call_type_ref_test.cpp` is owned by existing Idea 825, not by
  828; it is not evidence against 828's committed unary route.
- Exact return point: make the explicit 827 repair/close disposition. Its
  documented binary type-mismatch premise remains disproved, so do not begin
  a binary producer change without a newly observed in-scope first bad fact.

## Closure Record

Disposition: intentionally concluded; capability not claimed.

- The source's required binary definition/operation type-mismatch reproduction
  is disproved: the actual first failing operation was unary `fneg` with
  matching native parameter and operation types.
- Idea 828 repaired and accepted that unary route in `524b24f64`; clean
  detached before/after evidence also clears `frontend_hir_tests`.
- No new binary-LHS first bad fact exists. Remaining durable selector intent
  resumes under existing Idea 825 at Step 2, with its dirty implementation
  still unaccepted and separately accountable.
