# LIR Next Body Parameter Authority Handoff

Status: Closed
Parent Return: ideas/open/734_lir_to_new_bir_container_completeness.md after accepted Step 7.43 receiver commit `617a8fae9`
Type: producer/schema/verifier handoff for one function-body parameter row

## Goal

Publish and verify exactly one next native function-body parameter-use
authority row so idea 734 can later receive that one row into typed Raw BIR.

## Why This Exists

Idea 734 has accepted receiver coverage through Step 7.43, including the
DirectPointer and DirectScalar body-parameter rows already listed in its
history. Its source completion gate remains unmet because further valid
current-LIR parameter uses and other semantic families still lack evidenced
typed receiver dispositions. The next Raw-BIR receiver packet cannot choose a
parameter row from text, rendered operands, names, diagnostics, compatibility
mirrors, `monostate`, or testcase shape.

This idea owns the producer-side prerequisite only: find one next valid
function-body parameter-use row, publish native structured authority for it,
verify malformed forms fail closed, and record an exact handoff back to 734.

## In Scope

- Trace the current LIR body-parameter matrix after the accepted Step 7.43
  DirectScalar unary-`fneg` receipt.
- Select exactly one next function-body parameter-use row that is valid in
  current LIR and useful as the next 734 receiver packet.
- Publish native structured authority for that selected row, including the
  original current-function parameter value, owner, parameter index, type,
  ABI, explicit role, and consumer relation needed to prove the use.
- Add focused producer/verifier coverage for the positive row and nearby
  malformed missing, invalid, duplicate, foreign, owner/index/type/ABI/role,
  and consumer-incoherent authority.
- Record the exact handoff facts and proof needed for 734 to repair its
  runbook for one bounded Raw-BIR receiver packet.

## Out Of Scope

- Raw-BIR containers, builders, importer dispatch, verifier changes, or
  receiver tests.
- Reopening accepted DirectPointer or DirectScalar body-parameter receipts
  through Step 7.43.
- Selecting more than one parameter-use row.
- Memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, or other non-parameter families.
- Recovering authority from text, names, rendered operands, signatures,
  diagnostics, compatibility mirrors, `monostate`, or testcase shape.

## Acceptance Criteria

- Exactly one next valid body-parameter use has native structured authority
  sufficient for a later 734 Raw-BIR receiver.
- The selected row's authority records the original parameter identity, owning
  function, parameter index, type, ABI, explicit role, and selected consumer
  coherence without presentation recovery.
- The verifier rejects missing, invalid, duplicate, foreign, mismatched, and
  consumer-incoherent authority before printing or downstream use.
- Focused producer/verifier proof passes with neighboring malformed coverage.
- The final handoff names the exact selected row, accepted proof, commit, and
  734 return action.

## Reviewer Reject Signals

- Reject any Raw-BIR/importer/builder/receiver change in this idea.
- Reject reopening or weakening accepted body-parameter authority rows through
  Step 7.43.
- Reject selecting a row from operand spelling, names, rendered text,
  diagnostics, compatibility mirrors, `monostate`, or testcase-only shape.
- Reject publishing broad generic parameter authority without an exact
  selected consumer relation and verifier coverage.
- Reject absorbing memory/VA, aggregate/vector, module/type/global/metadata,
  instruction/terminator, inline-assembly, or more than one parameter row.
- Reject expectation downgrades, unsupported-to-supported label changes,
  helper-only refactors, or named-case-only checks claimed as capability
  progress.

## Closure Disposition

Closed as capability complete for this bounded producer/schema/verifier route.
The accepted handoff is exactly one native LIR body-parameter authority row:
`LirBinOp.scalar_lhs_parameter_authority` for a current-function
`DirectScalar` floating parameter used as the LHS of binary floating multiply
`fmul`.

Receiver tuple for
`ideas/open/734_lir_to_new_bir_container_completeness.md`: original parameter
`LirValueId`, current `LirFunction.link_name_id` owner, parameter index,
matching floating `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`,
and explicit `LirScalarBinaryParameterRole::Lhs`.

Consumer relation: `LirBinOp` opcode `fmul`; `lhs` is the same parameter
SSA/value as the authority tuple; `type_str` matches the authority type; and
`rhs` is a nonselected scalar operand. The existing carrier/emitter already
published the tuple. The accepted implementation tightened verifier consumer
coherence and added focused positive/malformed coverage for omitted, invalid,
duplicate, foreign, owner/index/type/ABI/role, non-`fmul`, LHS mismatch, type
mismatch, and RHS consumer incoherence.

Accepted producer/verifier proof:

- Step 2 commit `dcb6e1233` (`Prove fmul parameter authority`).
- Focused proof:
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$' ) > test_after.log 2>&1 && git diff --check`
- Broader frontend LIR stash-based regression guard passed with before 7/7,
  after 7/7, and no new failures.
- Direct code review found no blocker; commit `ac7ada4b6` repaired todo
  metadata while preserving the hook-managed reminder.

Return action: hand this exact one-row producer fact back to
`ideas/open/734_lir_to_new_bir_container_completeness.md` for one bounded Step
7.44 typed Raw-BIR receiver packet. Raw-BIR receipt remains separate and was
not implemented or claimed in 855.
