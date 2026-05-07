Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate HIR Late Substitution

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Migrate HIR Late Substitution", for HIR typed NTTP argument materialization.

- Added a structured `nttp_text_id` lookup in `HirTemplateArgMaterializer::resolve_explicit_typed_arg` that maps TextId carriers only through the current primary template parameter table before consulting NTTP bindings.
- Kept `debug_text` lookup in `nttp_bindings` available only for unstructured legacy value refs with no `nttp_text_id`.
- Made structured TextId misses avoid string binding lookup and either use literal value handling or fail closed.

## Suggested Next

Next coherent packet: supervisor review of the completed Step 4 late-substitution materialization/type-resolution slices, with attention to whether any remaining HIR NTTP debug-text authority is outside the migrated paths.

## Watchouts

- `find_bound_nttp_for_text_id` resolves only current-primary NTTP parameters; non-NTTP TextId matches, missing parameter-table entries, and missing bindings do not fall through to rendered spelling lookup.
- The legacy `debug_text` path still exists for older unstructured `TemplateArgRef` producers where `nttp_text_id == kInvalidText`.
- No focused test was added because the delegated `cpp_hir_` subset passed against the semantic materialization change without expectation downgrades.

## Proof

Ran the delegated proof exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_') > test_after.log 2>&1
```

Result: passed. The build completed and all 108 matching `cpp_hir_` tests passed. Proof log: `test_after.log`.
