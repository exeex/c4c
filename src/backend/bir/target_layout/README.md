# Target Layout

Status: converged design contract (unimplemented).

## Position and authority

Target-layout derivation is `C2`. It follows validation of one
[`TargetProfile` and complete `TargetFingerprint`](../../../target_profile/README.md) and the non-mutating
prepared-input gate at `C1`, consumes the `VerifiedPreparationInput` produced
for that exact Canonical/target pair, and precedes every planner. It is the sole
owner of the target's abstract register vocabulary and finite capacity model.
It does not inspect Canonical instructions or decide which value uses a slot.

The selected profile is identified by one `TargetFingerprint` covering the
architecture, triple/OS, backend ABI, relocation model, floating-point ABI,
enabled capabilities, layout-schema version, and concrete-mapping-table
version. Field-by-field equality of this fingerprint is required; architecture
name equality is insufficient.

## Input and immutable output

Input is the exact validated `TargetProfile`, its `TargetFingerprint`, and a
borrowing `VerifiedPreparationInput` carrying the complete Canonical
`PipelineStageStamp` and that same target fingerprint. The transaction derives
one immutable
`VerifiedTargetLayout` containing:

- ordered category, class, group, and abstract-slot identifiers;
- caller-saved, callee-saved, temporary, reserved, and allocatable sets;
- alias units and symmetric alias relations;
- finite capacities derived from eligible real slots (temporary sets never add
  capacity);
- group width, alignment, contiguity, legal-base, and alias rules;
- ABI argument/result eligibility and private abstract-to-concrete mapping
  domain; and
- a layout schema fingerprint, the exact `TargetFingerprint`, and the complete
  Canonical stage stamp.

The initial closed profile family is RV64 LP64 variants, AArch64 AAPCS64,
x86-64 SysV, and i686 SysV. Each supported fingerprint enumerates exact slots,
sets, capacities, aliases, groups, and ABI eligibility. No consumer may infer a
missing table from an architectural register count or a concrete register
name.

`VerifiedTargetLayout` is immutable after atomic publication. A reusable
target-only table template may be cached by target fingerprint, but the
published layout capability binds that template to the exact Canonical stage
stamp. Every downstream product embeds or references those exact stamp,
target, and layout fingerprints; a consumer rejects stale or merely
compatible-looking layouts.

## Consumers and non-ownership

ABI preparation consumes eligibility and class/group facts. Inline-assembly
preparation consumes the vocabulary to publish admitted source-description
tables. Constraint interpretation and allocation consume the same verified
layout later. MIR may use the private mapping domain only after allocated
publication has verified an abstract assignment.
E4's private frame-action draft consumes the exact layout's stack alignment,
displacement domains, frame-region/base vocabulary, and mapping-rule IDs.
After explicit action materialization, E4 publishes one immutable final
`FrameRealizationPlan`. F1 only applies that plan; it does not choose offsets,
bases, or stack adjustments.

This stage owns no Canonical revision, ABI classification of a particular
value, call plan, variadic plan, address strategy, helper selection, source
constraint interpretation, liveness, assignment policy, spill decision,
frame offset, machine opcode, or concrete BIR register identity. Exact frame
placements are owned only by the later E4 private product, never by this C2
layout or semantic BIR.

## Transaction and publication gate

Derivation occurs in one private transaction. Publication rejects unknown
profile combinations, duplicate IDs or mappings, overlapping reserved and
eligible units, asymmetric aliases, inconsistent capacities, impossible group
rules, empty required classes, ABI-ineligible mappings, or a fingerprint that
does not cover every selected target axis. A stale Canonical stage stamp or
target/stamp/`VerifiedPreparationInput` mismatch also rejects publication.
Failure publishes no layout or partial table. Inputs remain unchanged; any
Canonical stamp, target fingerprint, target-profile schema, or layout-schema
change invalidates the capability and every C3-C9 successor.
