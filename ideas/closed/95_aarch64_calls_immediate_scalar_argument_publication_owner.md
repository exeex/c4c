# AArch64 Calls Immediate Scalar Argument Publication Owner

## Goal

Isolate or consolidate the AArch64-local owner for immediate scalar call
argument publication, preserving prepared destination/source-home inputs,
concrete instruction spelling, and existing diagnostics.

## Why This Exists

The calls-owner audit identified immediate scalar argument publication as a
clear local subowner candidate. The cluster finds same-block cast producers,
extracts integer and floating immediate bits, chooses scalar GP/FP views, and
emits AArch64 inline-asm materialization lines for supported immediate cast
call arguments. It is target-local instruction spelling and operand rendering,
not a new source-publication authority.

## In Scope

- Audit and group the immediate scalar publication helpers:
  `find_same_block_cast_producer`, `integer_width_bits_for_type`,
  `immediate_integer_bits`, `scalar_fp_register_view`,
  `scalar_gp_register_view`, `append_materialize_fp_immediate`,
  `make_immediate_cast_call_argument_publication_lines`, and
  `make_immediate_cast_call_argument_publication_instruction`.
- Create a narrow local helper owner or tighten the current boundary around
  immediate cast call-argument publication.
- Preserve prepared destination registers, source homes, call argument plans,
  scalar views, concrete AArch64 materialization spelling, and diagnostics.
- Keep the integration point with `lower_before_call_immediate_binding` narrow
  and limited to consuming already-accepted immediate argument facts.

## Out Of Scope

- Moving publication-source authority, prepared call argument plans, or scalar
  producer selection out of their existing owners.
- Reworking general scalar producer lowering, before-call move ordering,
  aggregate byval lane transport, f128 carrier handling, or ordinary
  call-boundary record construction.
- Generalizing all constant materialization across ALU, memory, and calls
  without a separate owner/evidence idea.
- Changing supported immediate forms, unsupported diagnostics, assembly output,
  or selection status as a cleanup side effect.
- Claiming success through expectation rewrites or unsupported-path downgrades.

## Acceptance And Proof Expectations

- The resulting boundary is an AArch64-local immediate publication owner that
  consumes prepared destination/source-home inputs and does not select
  publication sources itself.
- Existing integer and floating immediate call-argument publication produces
  equivalent records, inline-asm materialization, diagnostics, and assembly.
- Focused proof covers integer immediates, floating immediates, same-block cast
  producer lookup, GP and FP destination views, unsupported or rejected
  immediate shapes when available, and nearby ordinary before-call moves.
- Build proof covers the AArch64 backend targets affected by `calls.cpp` and
  any new local helper files.
- If helper APIs or inline-asm record construction changes, use matching
  before/after logs for the focused test subset.

## Reviewer Reject Signals

- The implementation rederives publication-source, prepared call argument, or
  scalar producer facts under an immediate-publication helper name.
- Supported/unsupported immediate contracts, diagnostics, selection status, or
  assembly output change without explicit approval.
- The route claims progress through helper movement, helper renames, or test
  expectation rewrites while immediate publication remains mixed into the same
  before-call owner.
- General constant materialization across unrelated AArch64 owners is swept
  into this idea without a separate proof route.
- Proof covers only one named immediate call argument while integer, floating,
  GP-view, FP-view, rejected, and nearby non-immediate before-call paths remain
  unexamined.
- The route weakens or bypasses `lower_before_call_immediate_binding` checks
  instead of keeping it as the consumer of prepared immediate argument facts.

## Closure Note

Closed after implementation in:

- `bb3d889d7` (`Introduce AArch64 immediate scalar publication owner`)
- `42b97485f` (`Document immediate publication routing boundary`)

The route introduced the local
`ImmediateScalarCallArgumentPublicationOwner` in
`src/backend/mir/aarch64/codegen/calls.cpp`. The owner consumes prepared
destination and source-home facts for same-block immediate cast call arguments
and owns the calls-side immediate-publication rendering boundary: same-block
cast lookup, immediate integer bit extraction, GP and FP destination register
views, FP immediate scratch materialization, inline-asm line construction, and
immediate-publication instruction construction.

Intentionally outside this route:

- `lower_before_call_immediate_binding` remains the prepared immediate consumer
  and keeps authority for scalar literal lookup, destination facts, ordinary
  register immediate moves, synthetic `CallBoundaryMove` construction, and
  stack-slot immediate stores.
- `make_scalar_call_argument_immediate` remains the generic immediate operand
  adapter used by ordinary call-boundary and value moves.
- `make_call_boundary_move_instruction` remains generic record construction
  for all call-boundary move source kinds.
- Aggregate byval transport, f128 carriers, ordinary before-call moves,
  generic scalar materialization, indirect callee materialization, and edge
  publication remain outside this immediate-publication owner.

No remaining open ideas from the calls subowner audit chain remain. The prior
follow-ups were completed and closed as:

- `ideas/closed/93_aarch64_calls_stack_frame_slot_operand_owner.md`
- `ideas/closed/94_aarch64_calls_f128_carrier_operand_owner.md`

Proof:

- Focused before/after scope passed 7/7 before and 7/7 after.
- Close-time regression guard passed with non-decreasing pass-count mode:

```bash
cmake --build --preset default
ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result)$' --output-on-failure > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

The supervisor also reported broader validation on June 2, 2026:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

That broader backend scope passed 169/169.
