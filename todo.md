Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair HIR Late Substitution Domain Carriers

# Current Packet

## Just Finished

Step 4, "Repair HIR Late Substitution Domain Carriers", now routes HIR typed NTTP late substitution through a structured `TemplateArgRef` parameter-domain carrier instead of `nttp_text_id`, rendered parameter names, or `debug_text`.

- Added NTTP carrier fields on `TemplateArgRef`: owner template/function `TextId`, owner namespace context, parameter index, parameter kind, spelling `TextId`, plus existing structured value/deferred expression payload.
- Parser declaration annotation now fills the NTTP carrier for value refs that name the active template owner's NTTP parameter.
- HIR late NTTP lookup now requires carrier owner/index/kind to match the binding owner before using the string-keyed `NttpBindings` storage; `nttp_text_id` is spelling payload only.
- Removed the HIR typed-arg `debug_text -> nttp_bindings.find(debug_text)` semantic lookup and removed the missing-owner type `TextId -> rendered/current-primary type binding` acceptance paths in both HIR type resolution and materialization.
- Tightened structured type-pack substitution so same-owner structured carrier mismatches and missing-owner structured carriers cannot recover through debug or legacy names; the remaining `Args1#0` pack bridge is limited to complete foreign owner/index carriers and no-carrier legacy refs.

## Suggested Next

Next coherent packet: Step 5 should add focused domain-identity tests/probes for same-spelling NTTP parameters in different owners or nested instantiation contexts.

## Watchouts

- Remaining `find_template_typedef_binding` string-map reads are compatibility over the legacy `TypeBindings` storage after owner/index matching, or legacy no-structured-carrier fallback; removal condition is migrating `TypeBindings` itself to a structured domain-keyed storage.
- Remaining `nttp_bindings.find(param_name)` reads in HIR typed NTTP substitution are storage lookups after the new owner/index/kind carrier check; removal condition is migrating `NttpBindings` itself to structured domain-keyed storage.
- `Args1#0` / `Base#N` pack-series parsing remains compatibility-only for legacy explicit pack maps when no structured carrier exists or when a complete foreign owner/index carrier is carrying an outer pack binding through nested materialization; removal condition is replacing pack binding storage with structured parameter-domain pack entries.
- Remaining `debug_text` handling in touched HIR paths is display, deferred-expression syntax, numeric/bool literal parsing, no-carrier legacy type-argument compatibility, or the complete-foreign-carrier pack bridge above; it is not final domain-key authority for current-primary substitution.
- Step 5 should include tests that would fail if `TemplateArgRef::nttp_text_id`, parameter-name strings, `debug_text`, or current-primary lookup were still sufficient for NTTP late substitution.

## Proof

Ran delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_.*)$'`

Result: build succeeded; 111/111 tests passed. Proof log: `test_after.log`.
