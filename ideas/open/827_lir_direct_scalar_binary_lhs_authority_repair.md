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
uses a native direct-scalar parameter`. This is a distinct binary-LHS
producer/verifier first bad fact. Closed Idea 820's bounded `ull` publication
does not own or authorize this newly exposed native binary-LHS gap.

## In Scope

- Trace the native binary-operation producer that creates a `LirBinOp` whose
  `lhs` is a current-function `DirectScalar` parameter, and the matching
  `verify_scalar_binary_lhs_authority` check in
  `src/codegen/lir/verify.cpp`.
- Publish/check exactly the existing `LirBinOp.scalar_lhs_parameter_authority`
  tuple: native parameter `LirValueId`, current-function `LinkNameId` owner,
  parameter index, `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`,
  `Lhs` role, and exact `LirBinOp.lhs`/operation-type coherence.
- Add focused nearby positive and malformed-authority coverage, then run a
  fresh build and the exact focused frontend CTest.

## Out Of Scope

- Direct `LirSwitch.selector` authority, its `SwitchSelector` role, and all
  unaccepted Idea 825 code/test changes currently in the shared worktree.
- Raw-BIR/importer/builder work, generic parameter admission, ABI conversion,
  binary RHS, return-value, direct-pointer, or any second authority row.
- Reopening closed 820's `ull` publication, presentation-derived authority,
  test expectation weakening, or unrelated verifier redesign.

## Acceptance Criteria

1. The native binary-LHS producer emits the exact structured authority only
   when its LHS directly matches one current-function DirectScalar parameter.
2. The verifier rejects missing, invalid, duplicate, foreign,
   owner/index/type/ABI/role-invalid, and LHS or operation-type-incoherent
   authority; unselected forms remain fail closed.
3. A fresh `cmake --build --preset default` and exact
   `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
   provide focused acceptance evidence without crediting Idea 825's selector
   slice.
4. The handoff resumes Idea 825 exactly at Step 2 with its selector work still
   unaccepted and untouched by this blocker.

## Reviewer Reject Signals

- Reject a named-test bypass, expectation downgrade, or verifier exception
  that hides the same missing `scalar_lhs_parameter_authority` failure.
- Reject a helper rename, classification-only change, or display/type-string
  reconstruction claimed as native binary-LHS capability progress.
- Reject edits to direct switch-selector authority, the unaccepted Idea 825
  worktree slice, Raw-BIR/importer code, or any other parameter-use row.
- Reject acceptance without nearby positive and malformed coverage plus the
  exact focused CTest, or any claim that closed 820 already owns this gap.
