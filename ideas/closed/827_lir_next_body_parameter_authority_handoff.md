# LIR Next Body-Parameter Authority Handoff

Status: Closed
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Trace, publish, verify, and hand off exactly one next valid function-body
parameter-use authority row for a later typed Raw-BIR receiver.

## Why This Exists

734's accepted Step 7.39 received only closed 826's DirectScalar
truthiness-comparison-LHS row. Its next receiver row needs independently
structured producer authority; 734 cannot infer it from display text,
signatures, rendered operands, or compatibility fields.

## In Scope

- Trace the next valid, currently produced function-body parameter use after
  the accepted 826 row and select one bounded semantic consumer relation.
- Publish only the selected row's current-function value identity, owner,
  parameter index, type, ABI, explicit role, and required consumer coherence
  facts in native LIR.
- Verify missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority fails closed.
- Add focused same-feature positive and malformed-authority proof, then
  record an exact one-row receiver handoff to 734.

## Out Of Scope

- Any Raw-BIR container, builder, importer, or Raw-BIR verifier change.
- Reopening accepted DirectPointer or DirectScalar GEP, binary-LHS,
  binary-RHS, ReturnValue, switch-selector, or truthiness-comparison-LHS
  rows.
- Generic parameter admission, ABI conversion, declaration-only authority,
  presentation-derived identity, or a multi-row parameter sweep.
- Memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, and downstream receiver work.

## Acceptance Criteria

- Exactly one selected function-body parameter-use row has a structured,
  verifier-checked current-function contract and focused positive/negative
  producer proof.
- All nonselected parameter forms remain fail closed; no semantic fact is
  reconstructed from text, names, signatures, diagnostics, or rendered
  operands.
- The completion record names the exact receiver tuple, consumer relation,
  accepted proof, and 734 return action without claiming Raw-BIR receipt.

## Reviewer Reject Signals

- Reject a testcase-shaped selection, named-case-only producer shortcut, or
  any generic parameter authority claimed as this one-row capability.
- Reject expectation downgrades, weaker verifier/test contracts, or
  classification-only edits claimed as authority publication.
- Reject Raw-BIR/importer edits, broad ABI rewrites, or changes outside the
  selected producer/schema/verifier seam.
- Reject text-, name-, signature-, printer-, or diagnostic-derived identity,
  type, role, or consumer coherence.
- Reject retaining the exact missing authority behind a renamed carrier, or
  accepting a row without malformed/foreign/duplicate rejection coverage.

## Resumption Record: shared-worktree isolation prerequisite

Status: resumed at selected Step 2 after closed 828's accepted isolation
checkpoint.

- Last accepted progress: Step 1 selected only the native DirectScalar
  current-function parameter used unchanged as fixed direct-call argument 0.
  No Step 2 implementation, focused proof, or implementation commit exists.
- Interrupted step: Step 2 — Publish and verify the selected authority.
- Blocker: the necessary nearby proof surface
  `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp` is
  already dirty with preserved, unaccepted Idea 821/822 material; the related
  `src/codegen/lir/hir_to_lir/expr/binary.cpp` material is likewise dirty.
  Step 2 must neither overwrite, co-commit, nor absorb that material.
- Isolation completion: closed 828 preserved the dirty 821/822 slice in
  `review/828_preserved_821_822_frontend_slice.patch` at `7b0f8671e`; restore
  it only with `git apply review/828_preserved_821_822_frontend_slice.patch`.
  Its clean-route build and focused frontend test passed 1/1 with matching
  `test_before.log`/`test_after.log` evidence. That isolation accepts no
  821/822 or 827 semantics.
- Exact resumed action: **Step 2 — Publish and verify the selected authority**.
  Publish only an explicit `LirCallOp.structured_args[0]`
  authority tuple for an unchanged native DirectScalar parameter at a fixed
  direct call: matching current-function definition/value/owner/index/type/
  `DirectScalar` ABI, role `FixedDirectCallArgument0`, and coherence with
  structured argument 0 and fixed callee parameter 0. Reject missing, invalid,
  duplicate, foreign, owner/index/type/ABI/role, and consumer-incoherent forms.
  Do not admit later arguments, indirect/variadic/unspecified calls, pointer,
  spilled/load-derived, aggregate/vector, or any other parameter form.
- Required return proof: fresh `cmake --build --preset default`, then
  `ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'`.
  The isolation baseline is the conflict-free stash/pop clean pre-change run
  captured in `test_before.log`; it is not Step 2 acceptance proof.

## Completion Record

Close accepted: capability complete for this bounded LIR producer/schema/
verifier handoff only.

- Selected authority: `LirCallOp.structured_args[0]` carries
  `LirFixedDirectCallArgumentParameterAuthority` with the matching
  current-function parameter definition/value, `owner`, `parameter_index`,
  `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and
  `LirFixedDirectCallArgumentParameterRole::FixedDirectCallArgument0`.
- Selected consumer relation: only an unchanged native current-function
  DirectScalar parameter passed as structured direct-call argument 0, where
  that structured argument's SSA value/type agree with the authority and with
  fixed callee parameter 0 of a direct, non-variadic, specified call.
- Accepted implementation and proof: `96a6bb20f` (`Publish fixed direct-call
  parameter authority`); fresh `cmake --build --preset default` and
  `ctest --test-dir build --output-on-failure -R
  '^frontend_lir_call_type_ref$'` passed 1/1 before and after. Positive plus
  missing, invalid, duplicate matching native definition, foreign,
  owner/index/type/ABI/role, and consumer-incoherent rejection coverage is
  included. A fresh `ctest --test-dir build --output-on-failure -R
  '^backend_'` checkpoint also passed 6/6.

734's sole corresponding next action is **Step 7.40 — Receive the one
827-authorized fixed-direct-call argument-0 DirectScalar body-parameter
authority row**. It may add only that tuple's typed Raw-BIR call-argument
destination, importer dispatch, reachable verifier path, and transactional
positive/malformed-authority coverage. This producer handoff neither performs
nor claims Raw-BIR receipt, and it does not complete 734's broader source
intent.
