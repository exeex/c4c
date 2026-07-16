# Current Packet

Status: Active
Source Idea Path: ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Populate canonical refs for aggregate function signatures

## Just Finished

- Step 1 accepted: `Lowerer::qtype_from` is the HIR `QualType` producer
  (`src/frontend/hir/hir_types.cpp:469`); function returns and parameters use
  it at `hir_functions.cpp:543` and `:1157`. HIR registration accessors are
  `Module::register_aggregate_definition` / `aggregate_ref_for_definition`
  (`hir_ir.hpp:2611` / `:2618`), but registration is currently downstream in
  `lir::lower` (`hir_to_lir.cpp:1804-1805`), too late for occurrence creation.

## Suggested Next

- Establish HIR-side definition registration before `qtype_from` resolves the
  existing definition-backed ref, then populate supported function return and
  parameter occurrences. Leave the ref unset when no direct registered,
  module-owned, complete HIR ref exists.

## Watchouts

- This blocker owns HIR occurrence fact production only. Do not migrate LIR
  lowering or recover aggregate identity from legacy owner keys, tags, text, or
  parser pointers.
- Fail closed for missing, invalid, foreign, non-module-owned, or incomplete
  refs; do not reconstruct identity.

## Proof

- Lifecycle slice: structural/linkage inspection only. Code-bearing packets
  must select and record fresh focused proof before acceptance.
