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
