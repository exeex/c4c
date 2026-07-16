# Current Packet

Status: Active
Source Idea Path: ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md
Source Plan Path: plan.md
Current Step ID: 2a
Current Step Title: Establish a direct HIR aggregate-ref construction input

## Just Finished

- Step 1 accepted: `Lowerer::qtype_from` is the HIR `QualType` producer
  (`src/frontend/hir/hir_types.cpp:469`); function returns and parameters use
  it at `hir_functions.cpp:543` and `:1157`. HIR registration accessors are
  `Module::register_aggregate_definition` / `aggregate_ref_for_definition`
  (`hir_ir.hpp:2611` / `:2618`), but registration is currently downstream in
  `lir::lower` (`hir_to_lir.cpp:1804-1805`), too late for occurrence creation.

## Suggested Next

- Register the module-owned `HirStructDef` at its HIR construction seam and
  retain its issued `HirAggregateRef` in a HIR-only carrier. Add an explicit
  optional ref input to `qtype_from`; it may validate and copy only this input.
  It must not derive canonical identity from `TypeSpec`, `record_def`, a
  `Node*` map, owner keys, tags, or text. Leave the ref unset when the direct
  input is absent, invalid, foreign, or incomplete.

## Watchouts

- This blocker owns HIR occurrence fact production only. Do not migrate LIR or
  recover aggregate identity from legacy owner keys, tags, text, parser
  pointers, or a Node-to-HIR map.
- Fail closed for missing, invalid, foreign, non-module-owned, or incomplete
  refs; do not reconstruct identity.

## Proof

- Pre-change `ctest --test-dir build -j --output-on-failure -R
  '^frontend_hir_tests$'` is in `test_before.log` and reproduces the missing
  canonical HIR aggregate-ref failure. The code-bearing packet must select and
  record fresh focused proof before acceptance.
