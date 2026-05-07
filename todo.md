Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Clean Compatibility Boundaries

# Current Packet

## Just Finished

Step 6, "Validate And Clean Compatibility Boundaries", inventoried the remaining parser/HIR NTTP and type binding compatibility surfaces and made this a todo-only cleanup packet.

- No code cleanup was safe in this packet: the remaining string-pair overloads, HIR string maps, token-spelling comparisons, TextId-only checks, and `debug_text` routes still have active compatibility, display, syntax, or legacy-storage roles.
- Remaining parser `vector<pair<string, TypeSpec>>` and `vector<pair<string, long long>>` APIs are compatibility overloads for tests and legacy callers. Active parser instantiation paths build `ParserTemplateBindingSet` with owner/index/TextId metadata before deferred NTTP evaluation.
- Parser token-spelling comparisons remain syntax/compatibility fallback only when authoritative structured metadata is absent; structured NTTP metadata gates the old spelling fallback in deferred expression and captured-argument evaluation.
- HIR `TypeBindings` and `NttpBindings` remain string-keyed storage. The touched late-substitution paths now enter those maps only after a structured owner/index/kind carrier selects the parameter name, or through explicitly no-carrier legacy compatibility.
- Remaining `TemplateArgRef::debug_text` uses in touched HIR paths are display rendering, deferred-expression syntax, numeric/bool literal parsing, no-carrier legacy type-argument compatibility, or the bounded foreign-owner pack bridge; Step 5 probes guard against using debug/rendered spelling after a structured NTTP carrier miss.

## Suggested Next

Next coherent packet: supervisor should decide whether to request reviewer acceptance for the completed Step 4-6 route or route plan-owner lifecycle handling for any remaining source-idea follow-up.

## Watchouts

- Removal condition for parser string-pair overloads: migrate the remaining direct compatibility tests and any legacy external helper callers to `ParserTemplateBindingSet`, then delete the overloads and the legacy conversion bridge.
- Removal condition for HIR `TypeBindings` / `NttpBindings` string-map reads: migrate binding storage itself to domain-keyed type and NTTP maps; current structured carriers still project to parameter names because those maps remain the storage contract.
- Removal condition for `Args1#0` / `Base#N` pack-series parsing: replace pack binding storage with structured parameter-domain pack entries.
- Removal condition for `debug_text` in HIR materialization/type resolution: keep only diagnostics/display and deferred-expression syntax after structured pack and deferred NTTP expression carriers no longer need string syntax transport.
- `test_baseline.log` and `test_before.log` existed at the root alongside the required `test_after.log`; this packet did not touch or create them.

## Proof

Ran delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_.*)$'`

Result: build succeeded; 111/111 tests passed. Proof log: `test_after.log`.
