# Stack-Passed Parameter-Home Publication

Source Parent: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Handoff: docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
Owning Layer: producer ABI/prealloc contract

## Goal

Publish prepared ABI homes for stack-passed parameters, including storage kind,
stack offsets, widths, alignments, and the call/callee contract needed by RV64
object emission.

## Why This Exists

Idea 424 classified `src/20001017-1.c` and the remaining
`unsupported_param_home` rows as producer-contract gaps. The 13-argument call
is visible, but stack-passed parameter homes are missing or offsetless:

- `RV64 object route requires all parameters in supported GPR or prepared FPR register homes`

Representative evidence:

- Primary row: `src/20001017-1.c`
- Remaining rows: `src/20010518-1.c`, `src/pr27073.c`, `src/pr69447.c`
- Bucket: `unsupported_param_home`
- Artifacts:
  `build/agent_state/424_step2_infrastructure_classification/20001017-1/`
- Handoff docs:
  `docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`
  and
  `docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md`

Known missing authority from the handoff:

- Callee homes for `%p.B`, `%p.fdB`, `%p.b`, `%p.C`, and `%p.fdC` are
  `kind=none` or `encoding=none`.
- Caller stack arguments 8-12 have stack-slot bindings without destination
  offsets or register/bank homes.
- RV64 lacks explicit stack-passed parameter offsets, widths, alignment, and
  storage-kind authority.

## In Scope

- Producer ABI/prealloc publication of stack-passed parameter homes for
  ordinary-C calls beyond the register argument set.
- Caller and callee contract facts for stack storage kind, destination offset,
  width, and alignment.
- Prepared-printer evidence that stack-passed homes are visible to RV64.
- Focused ordinary-C coverage for stack-passed arguments once the producer
  contract is coherent.
- Fail-closed diagnostics when stack homes remain missing or ambiguous.

## Out Of Scope

- RV64 inference of stack argument homes from argument index, ABI folklore, or
  source call shape.
- Broad varargs, F128, FPR-only, aggregate ABI, or dynamic stack work unless a
  later source idea scopes it explicitly.
- RV64 consumer implementation beyond proving that the producer contract is
  coherent and consumable.
- Expectation, unsupported-marker, allowlist, runtime-comparison, or scan
  accounting changes.

## Acceptance Criteria

- Prepared call/parameter facts publish explicit stack-passed homes for the
  representative ordinary-C call.
- `src/20001017-1.c` or a focused equivalent no longer fails because stack
  parameter homes are `none` or offsetless.
- Remaining `unsupported_param_home` rows are reclassified with evidence: they
  either consume the new contract, expose a narrower producer gap, or stay
  fail-closed for an explicitly out-of-scope ABI feature.
- RV64 does not reconstruct offsets from argument indexes.
- Proof includes focused prepared dumps and the relevant backend subset.

## Reviewer Reject Signals

Reject any route or slice that:

- special-cases `src/20001017-1.c`, a 13-argument call, argument indexes 8-12,
  or the listed parameter names
- reconstructs parameter homes in RV64 from ABI formulas without prepared
  producer authority
- combines producer parameter-home publication with broad RV64 call lowering,
  varargs, F128, or unrelated ABI repairs
- changes expectations, unsupported markers, allowlists, runtime comparison,
  or pass/fail accounting instead of publishing homes
- claims progress through printer-only output, diagnostics renames, or
  classification edits without authoritative prepared contract records
- retains the old `requires all parameters in supported GPR or prepared FPR
  register homes` failure mode behind a renamed check

## Closure Notes

Closed after producer/lowering, prealloc, and RV64 object admission published
and consumed explicit stack-passed parameter homes for ordinary-C scalar
formals beyond the current RV64 register-argument budget.

Completed evidence:

- Ordinary-C stack call/formal ABI metadata is published before prealloc.
- Authorized fixed stack-passed formals without a unique prior spill-slot home
  now receive prepared stack homes from producer authority.
- RV64 object admission consumes only explicit prepared `passed_on_stack` and
  frame-slot facts for scalar stack-passed formals.
- Focused coverage proves accepted I64 and F64 stack-passed formal homes and
  fail-closed malformed offsets, frame slots, source kinds, missing
  `passed_on_stack`, memory class, and F128.
- `src/20001017-1.c`, `src/20010518-1.c`, `src/pr27073.c`, and
  `src/pr69447.c` no longer expose the old `unsupported_param_home` gap when
  prepared homes are coherent.

Validation:

- Focused stack-parameter subset passed, 22/22.
- Backend subset passed, 345/345.
- Regression guard passed against `test_before.log`: before 331/331, after
  345/345, no new failures or slow tests.

Remaining non-goals:

- `src/20001017-1.c` still hits the pre-existing `unsupported_stack_frame ...
  fpr:fs1` stack-frame diagnostic.
- `src/20010518-1.c`, `src/pr27073.c`, and `src/pr69447.c` now pass the
  parameter-home gate and later hit `unsupported_move_bundle_target_shape` for
  generic stack-to-stack value moves.
