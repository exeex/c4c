Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate HIR Late Substitution

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Migrate HIR Late Substitution", for HIR member-typedef/type-resolution template argument rebinding.

- Added a structured TypeSpec carrier lookup in HIR template argument materialization that resolves owner/index/TextId metadata through the current primary template parameter table.
- Changed `find_bound_type_for_param_ref` so structured carriers bind only through that primary-template domain lookup, then fail without consulting debug-name or legacy tag strings.
- Changed type-pack lookup to expand structured pack parameter refs through the same primary-template parameter name and reserve debug-name/string candidates for unstructured legacy TypeSpec inputs.

## Suggested Next

Next coherent packet: supervisor review of the Step 4 materialization/type-resolution slices together, with attention to whether any remaining HIR late-substitution debug-text authority is outside the already migrated paths.

## Watchouts

- `type_param_name_for_ref` intentionally rejects NTTP parameters for type binding and treats owner mismatch as structured lookup failure, not as permission to try debug text.
- `tag_text_id` is treated as structured carrier metadata only for `TB_TYPEDEF` parameter refs in these helpers; unstructured legacy fallbacks remain available only when no structured carrier fields are present.
- No focused test was added because the delegated `cpp_hir_` subset already covers the materialization regressions hit by this slice without expectation downgrades.

## Proof

Ran the delegated proof exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_') > test_after.log 2>&1
```

Result: passed. The build completed and all 108 matching `cpp_hir_` tests passed. Proof log: `test_after.log`.
