# Current Packet

Status: Active
Source Idea Path: ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md
Source Plan Path: plan.md
Current Step ID: 2a
Current Step Title: Establish a direct HIR aggregate-ref construction input

## Just Finished

- Step 2a complete: ordinary and template-instantiated module-owned aggregate
  definition construction now store their issued `HirAggregateRef` on
  `HirStructDef`; `qtype_from` accepts only an explicit optional ref and copies
  it only when complete and owned by its module. Focused coverage proves valid
  direct input and missing, incomplete, and foreign inputs fail closed.

## Suggested Next

- Step 2b should thread the stored direct `HirStructDef::aggregate_ref` into
  function return and parameter construction call sites, without recovering
  identity from legacy metadata.

## Watchouts

- This blocker owns HIR occurrence fact production only. Do not migrate LIR or
  recover aggregate identity from legacy owner keys, tags, text, parser
  pointers, or a Node-to-HIR map.
- Existing `qtype_from` callers still omit the new ref by default; Step 2b owns
  the function-signature propagation work.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure
  -R '^frontend_hir_tests$' | tee test_after.log` passed; proof log:
  `test_after.log`.
