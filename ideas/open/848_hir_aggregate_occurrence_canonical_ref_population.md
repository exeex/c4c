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

## Accepted Discovery Checkpoint

Step 1 established the producer seam without implementation changes:

- `Lowerer::qtype_from` (`src/frontend/hir/hir_types.cpp:469`) constructs HIR
  `QualType`, retaining legacy owner identity; function return and parameter
  construction reaches it at `src/frontend/hir/hir_functions.cpp:543` and
  `:1157`.
- `Module::register_aggregate_definition` and
  `aggregate_ref_for_definition` are the existing HIR definition/ref seams
  (`src/frontend/hir/hir_ir.hpp:2611` and `:2618`).
- Current registration happens only later in downstream `lir::lower`
  (`src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1804-1805`), after the HIR
  occurrence is built. Step 2 must register the definition on the HIR side
  before `qtype_from` resolves that existing definition-backed ref.

The producer contract fails closed: a module-owned `HirStructDef` must issue
and retain the canonical `HirAggregateRef` at its HIR construction seam, and
an occurrence constructor must receive that ref as an explicit HIR construction
input. Leave `aggregate_ref` unset when that direct input is absent, invalid,
foreign, or incomplete. `qtype_from` must not derive the ref from `TypeSpec` or
its parser-owned `record_def`; no `Node*` map, legacy owner-key, tag,
parser-pointer, rendered-text, or reconstructed lookup may supply canonical
identity.

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
