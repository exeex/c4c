Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate HIR Late Substitution

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Migrate HIR Late Substitution", regression repair for HIR typed template argument materialization.

- Restored owner-mismatched structured type-pack substitution when the incoming carrier refers to a foreign pack and the available binding contract is an explicit pack series such as `Args1#0`.
- Kept plain structured type carriers from falling back to direct debug-text type binding lookup; the compatibility path only scans for `Base#N` pack bindings.
- Left `frontend_hir_lookup_tests` assertions unchanged because the existing non-downgrading regression test now passes.

## Suggested Next

Next coherent packet: supervisor review of the completed Step 4 late-substitution materialization/type-resolution slices, with attention to whether any remaining HIR NTTP debug-text authority is outside the migrated paths.

## Watchouts

- `find_bound_nttp_for_text_id` resolves only current-primary NTTP parameters; non-NTTP TextId matches, missing parameter-table entries, and missing bindings do not fall through to rendered spelling lookup.
- The legacy `debug_text` path still exists for older unstructured `TemplateArgRef` producers where `nttp_text_id == kInvalidText`.
- The restored foreign structured-pack compatibility is intentionally limited to parsed pack binding names like `Args1#0`; it does not accept a plain `Args1` debug spelling as a direct type binding for structured carriers.

## Proof

Ran the delegated proof exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_hir_)') > test_after.log 2>&1
```

Result: passed. The build completed and all 109 matching tests passed, including `frontend_hir_lookup_tests`. Proof log: `test_after.log`.
