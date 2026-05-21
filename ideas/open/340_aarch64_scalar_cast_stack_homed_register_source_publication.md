# AArch64 Scalar Cast Stack-Homed Register Source Publication

Status: Parked - scope satisfied; close deferred
Created: 2026-05-20
Split From: ideas/open/295_backend_regex_failure_family_inventory.md
Reopens Boundary From: ideas/closed/338_aarch64_scalar_cast_register_source_operand_facts.md

## Goal

Repair the post-339 AArch64 scalar cast selected-node path where a stack-homed
source value has a prepared consumer stack-to-register move, but the selected
`sign_extend` cast still reaches machine printing without a structured register
source operand.

## Why This Exists

The post-339 umbrella inventory found fresh generated/prepared evidence for
`00143` that contradicts the closure boundary of idea 338 for a narrower
subcase. This should not mutate the closed archive, but it does require a new
focused owner for the reopened operand-fact boundary.

The current first bad fact is compile-stage selected machine-node printing:
`c_testsuite_aarch64_backend_src_00143_c` fails before assembly with
`family=scalar opcode=sign_extend` and the diagnostic that a scalar cast node
requires a structured register source operand.

Prepared evidence shows the source value is stack-homed and a consumer
stack-to-register move was prepared:

- `%t76 = bir.select ... i16`
- `%t81 = bir.sext i16 %t76 to i32`
- `%t76 value_id=308` is `kind=stack_slot`
- `%t81 value_id=388` is `kind=register reg=x13`
- the move bundle before the failing selected instruction contains
  `move from_value_id=308 to_value_id=388 destination_storage=register
  reason=consumer_stack_to_register`

Therefore the prepared route has evidence for the needed register
materialization, but selected AArch64 scalar cast lowering or publication does
not expose that moved register as the structured source operand consumed by the
printer. This is not the closed idea 339 local storage/writeback sizing owner.

## In Scope

- Localize where a selected scalar cast loses the prepared register source for
  a stack-homed input after a consumer stack-to-register move exists.
- Repair publication, normalization, or selected-node handoff so scalar casts
  consume the prepared register source as a structured operand.
- Cover the stack-homed `i16` select feeding `sext` to `i32` shape without
  hard-coding `00143`, value ids, instruction numbers, registers, or block
  names.
- Add or update focused backend coverage for selected scalar cast source
  publication after stack-to-register consumer moves.
- Prove the representative advances past the current scalar cast structured
  register-source printer diagnostic or passes.

## Out Of Scope

- Rewriting closed idea 338 or mutating files under `ideas/closed/`.
- Reopening idea 339 local scalar storage/writeback sizing without fresh
  generated-code evidence that the current first bad fact moved there.
- Runtime value correctness, frame-layout sizing, compare lowering,
  timeout/output-storm handling, runner behavior, expectations, unsupported
  classification, proof-log policy, or CTest registration.
- Fixing only `00143`, one value id, one selected instruction index, one
  register, one source line, one diagnostic string, or one emitted instruction
  sequence.

## Acceptance Criteria

- The missing structured register source for the representative selected
  scalar cast is localized to a concrete source publication, move consumption,
  normalizer, or selected-node handoff boundary.
- Focused backend coverage proves scalar casts can publish a structured
  register source when their original source is stack-homed but a prepared
  consumer stack-to-register move exists.
- `c_testsuite_aarch64_backend_src_00143_c` no longer fails with the scalar
  cast structured register-source printer diagnostic.
- Any remaining `00143` failure is reclassified by its new first bad fact
  before this idea is closed.
- No expectation, runner, timeout, unsupported, CTest-registration, or
  proof-log-policy change is used to claim progress.

## Lifecycle Note 2026-05-20

Runbook execution exhausted the scalar-cast source-publication scope. Step 2
repaired `make_prepared_consumer_register_source` and added focused backend
coverage for selected scalar casts whose original source is stack-homed but has
a prepared consumer stack-to-register move. Step 3 confirmed the old
`scalar cast node requires a structured register source operand` diagnostic is
absent from the focused proof.

Closure was not accepted in this lifecycle pass because the required
regression guard reported no new failures but failed its strict pass-count
increase rule: `test_before.log` and `test_after.log` both record 6/7 passing.
The active lifecycle state was therefore switched to the separately scoped
follow-up idea
`ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md`.

The remaining `00143` residual is out of scope for this idea: prepared BIR
contains fallthrough fixed-offset local loads/stores for the Duff-device copy,
but generated AArch64 for the matching fallthrough labels only advances the
`from`/`to` pointer locals and omits the `ldrh`/`strh` data copy.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, `%t76`, `%t81`, value ids 308 or 388, `x13`, block
  16, instruction 158, one diagnostic string, one selected instruction name,
  or one source line instead of repairing scalar cast register-source
  publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to reduce the failure count;
- claims progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- broadens into idea 339 local storage/writeback sizing, runtime value
  correctness, frame layout, compare lowering, aggregates, variadics, or
  semantic `lir_to_bir` admission without fresh evidence that the scalar cast
  first bad fact moved there;
- leaves selected scalar cast nodes reaching AArch64 printing without a
  structured register source operand behind a renamed abstraction;
- proves only the named representative while nearby stack-homed scalar cast
  source-publication behavior remains unexamined.
