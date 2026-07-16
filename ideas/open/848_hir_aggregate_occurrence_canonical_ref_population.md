# HIR Aggregate Occurrence Canonical Ref Population

Status: Open
Type: upstream HIR producer prerequisite for aggregate occurrence lowering
Parent Return: 838 Step 2

## Goal

Populate every supported aggregate `QualType` occurrence with the canonical,
definition-backed `HirAggregateRef` needed by downstream LIR lowering, starting
with function return and parameter occurrences.

## Why This Exists

The LIR aggregate store now requires canonical HIR aggregate facts. The carrier
exists, but HIR construction leaves `QualType::aggregate_ref` unset and only
preserves legacy owner identity. Enforcing the canonical contract consequently
breaks supported aggregate function signatures. This is an upstream producer
gap, not an LIR M4--M6 consumer migration.

## In Scope

- Trace aggregate definition registration and `QualType` construction to the
  narrow HIR producer seam that can attach the definition-backed
  `HirAggregateRef`.
- Populate the canonical ref for supported aggregate occurrences, at minimum
  function return and parameter occurrences, without reconstructing identity
  from tags, text, parser pointers, or owner keys.
- Define and prove explicit behavior for missing, invalid, and foreign refs;
  preserve module ownership boundaries and fail closed where canonical
  definition facts are unavailable.
- Add nearby same-feature HIR/lowering coverage for function signatures and
  the invalid/foreign/missing boundary needed by the producer contract.

## Out Of Scope

- LIR `lir_owned_type_spec` function-signature occurrence-producer migration,
  LIR aggregate-store consumer migration, or changes to 838's M4--M6 scope.
- Retaining or introducing legacy owner-key, tag, rendered-text, parser-pointer,
  or reconstructed-lookup fallback for canonical aggregate identity.
- Completing 836 or 831, changing their returns, or broad nominal-family,
  vector, scalar, union, verifier, printer, or universal-model migration.

## Acceptance Criteria

- Supported aggregate function return and parameter `QualType` occurrences
  carry a definition-backed `HirAggregateRef` that agrees with the registered
  definition and module.
- Missing, invalid, and foreign occurrence refs have explicit, fail-closed
  behavior; no silently recovered legacy identity becomes canonical authority.
- Focused coverage demonstrates named and representative nontrivial aggregate
  occurrence propagation plus invalid-boundary behavior, followed by a fresh
  build and proportional backend proof.
- The resulting handoff gives 838 an exact, evidence-backed return: retry only
  its bounded `lir_owned_type_spec` function-signature producer migration.

## Reviewer Reject Signals

- Reject tag, parser-pointer, rendered-text, or reconstructed owner-key lookup
  used to populate or recover `HirAggregateRef`.
- Reject a named-test-only patch, weakened invalid/foreign/missing behavior, or
  an expectation downgrade presented as propagation progress.
- Reject changes that migrate LIR function-signature lowering, LIR store
  consumers, verifier/printer behavior, or unrelated nominal families under
  this HIR producer prerequisite.
- Reject retaining the existing unset-`aggregate_ref` failure mode behind a new
  helper name or claiming completion without definition-backed occurrence facts.
