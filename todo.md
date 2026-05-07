Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate HIR Late Substitution

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Migrate HIR Late Substitution", for HIR member-typedef/type-resolution template argument rebinding.

- Added structured carrier checks for HIR `TemplateArgRef` rebinding so TypeSpec and NTTP TextId metadata are authoritative before any string fallback.
- Reworked late member-typedef substitution to resolve forwarded NTTP values only when structured `nttp_text_id` matches the current template owner's NTTP parameter table, and to gate `debug_text` binding lookups behind legacy-unstructured checks.
- Kept generated `debug_text` assignment as display text while preventing structured template args from using `debug_text` as binding authority.

## Suggested Next

Next coherent packet: audit adjacent HIR template materialization/struct-instantiation paths for remaining `TemplateArgRef::debug_text` authority and migrate any structured-carrier cases with the same compatibility boundary.

## Watchouts

- The retained `debug_text` binding lookups in `type_resolution.cpp` are now compatibility fallbacks for unstructured legacy `TemplateArgRef` inputs.
- TextId-only NTTP args are spelling identity, not semantic binding authority; NTTP rebinding uses the existing `NttpBindings` name map only after the current template owner table matches the TextId to an NTTP parameter name.
- `clang-format` was not available in this environment, so formatting was kept manually aligned with the surrounding file.

## Proof

Ran the delegated proof exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_') > test_after.log 2>&1
```

Result: passed. The build completed and all 108 matching `cpp_hir_` tests passed. Proof log: `test_after.log`.
