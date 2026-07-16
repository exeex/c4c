# LIR Direct-Call Structured Argument Identity Prerequisite

Status: Open
Type: bounded native LIR construction/schema/verifier prerequisite
Blocked Parent: `ideas/open/829_lir_next_body_parameter_authority_handoff.md`

## Goal

Establish one bounded, native LIR structured identity and type relation for
argument 1 of a valid direct, non-variadic, specified two-parameter call, so
829 can later determine whether its existing parameter definition can be
published without recovering semantics from presentation data.

## Why This Exists

829's selected direct-call parameter relation cannot currently be represented
structurally: `arg_type_refs` is empty and `structured_args[1].operand` is
non-SSA/text-only, despite native parameter definitions and callee signature
entries. There is no authoritative value/type relation for 829 to consume.

## In Scope

- Trace the native LIR construction seam that creates the selected direct-call
  argument and decide the smallest structural producer representation that
  preserves argument-1 identity and exact type without text recovery.
- Produce and natively verify that one direct/non-variadic/specified
  argument-1 relation, including its coherence with the call and callee
  parameter-1 type.
- Add focused positive and malformed structural-relation proof sufficient to
  establish the producer prerequisite.
- Record the exact produced relation and return 829 to Step 2; do not publish
  a body-parameter authority tuple here.

## Out Of Scope

- Publishing `LirCurrentFunctionBodyParameterDefinition` authority,
  `FixedDirectCallArgument1` role, or any 829 malformed-authority contract.
- Raw-BIR/container/importer/receiver work, 734 receipt, generic call
  arguments, other indices, indirect/variadic/unspecified calls, ABI
  conversion, or parser-derived semantic recovery.
- Names, signatures, printed operands, diagnostics, compatibility fields, or
  tests that infer the missing semantic relation from presentation text.
- Unrelated 821/822 work and all previously accepted parameter rows.

## Acceptance Criteria

- One valid argument-1 direct-call case has an existing native structural
  identity/type relation produced at its LIR construction seam and checked by
  native verification; it is not reconstructed from text or signatures.
- Focused positive proof exercises that relation, and malformed/missing or
  incoherent relation proof fails closed at the producer boundary.
- The completion record states precisely whether the relation is sufficient
  for 829's existing parameter-definition tuple, and names 829 Step 2 as the
  return action. It does not claim any body-parameter authority was published.

## Reviewer Reject Signals

- Reject any text-, name-, signature-, printer-, diagnostic-, or
  parser-shaped reconstruction presented as structural identity or type.
- Reject a generic call-argument rewrite, other-index sweep, ABI conversion,
  or receiver work claimed as this bounded prerequisite.
- Reject test-only fixture shaping, expectation downgrades, or weaker
  contracts in place of native producer/verifier proof.
- Reject a renamed compatibility field that retains the non-SSA/text-only
  argument relation or lacks malformed/incoherent rejection.
- Reject publishing the 829 authority tuple or `FixedDirectCallArgument1`
  role before 829 is reactivated and independently validates the prerequisite.

## Resumption Record: rejected baseline-family decomposition blocker

Status: parked by lifecycle switch to
`ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`.

- Last accepted progress: Steps 1 and 2 are complete. Step 2's accepted
  implementation commit is `f0fc85e4f` (`Preserve direct-call argument
  identity`), with focused `frontend_lir_call_type_ref` proof passing 1/1.
- Interrupted step: Step 3 — **Diagnose and repair the rejected post-commit
  baseline**. No Step 3 repair or acceptance is claimed.
- Rejected baseline evidence: the accepted baseline at `8418036b` was
  3038/3038; the candidate at `f0fc85e4f` was 3024/3038. The exact 14
  failures reproduce with `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(frontend_hir_tests|llvm_gcc_c_torture_src_(20090113_2|930719_1|931012_1|950512_1|961112_1|comp_goto_1|pr23604|pr28289|pr37780|pr43385|pr46909_2|pr51323|pr88714)_c)$'
  > test_after.log`.
- Blocker outside this idea's scope: the 13 torture failures report the
  pre-existing `LirCmpOp.truthiness_lhs_parameter_authority` verifier route,
  unchanged in `f0fc85e4f^`; a clean isolated `f0fc85e4f^` build with
  `ENABLE_C4C_BACKEND=ON` independently reproduces the `frontend_hir_tests`
  segfault. Neither belongs to this idea's bounded direct/non-variadic,
  specified argument-1 relation.
- Lifecycle decision: the HIR segfault and truthiness-parameter verifier
  family do not honestly share one implementation owner. New Idea 831 owns
  only their evidence-preserving first-owner decomposition and the resulting
  dependency order; it must create separately scoped repair routes before
  either repair is attempted.
- Exact return point: after the required repair route(s) produce accepted
  comparable full-suite evidence with no new baseline problem, reactivate 830
  at unchanged Step 3 solely to accept that gate. Then execute existing Step
  4 focused completion and return to 829 Step 2. Do not make an 830 code
  change while this blocker route is active.
