Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Binding Carriers

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Define Structured Binding Carriers", for the first parser-side packet.

- Added distinct parser binding-domain carriers in `src/frontend/parser/parser_types.hpp`: `ParserTemplateTypeBinding`, `ParserTemplateNttpBinding`, shared `ParserTemplateParameterBindingKey`, parameter kind, and `ParserTemplateBindingSet`.
- Extended parser NTTP metadata with owner template key and parameter index, and populated those fields from parser template parameter metadata in `src/frontend/parser/impl/types/base.cpp`.
- Kept the legacy `vector<pair<string, TypeSpec>>` and `vector<pair<string, long long>>` API wrappers in `src/frontend/parser/parser.hpp` and `src/frontend/parser/impl/types/template.cpp`, but made them convert into `ParserTemplateBindingSet` before deferred evaluator lookup.
- Added a structured evaluator overload that resolves type and NTTP bindings through the typed carriers first. NTTP fallback string matching remains compatibility-only when no authoritative structured NTTP metadata is present.

## Suggested Next

Next coherent packet: migrate parser deferred evaluation call sites that can already supply owner/index metadata to pass `ParserTemplateBindingSet` directly instead of relying on the legacy string-pair wrapper conversion.

## Watchouts

- Legacy parser wrapper conversion can only infer full owner/index metadata where an owner key or NTTP metadata is already available. Type-binding call sites still need a direct structured path in a later packet to avoid relying on spelling-only compatibility fields.
- The structured evaluator intentionally keeps legacy NTTP string fallback only when no authoritative structured NTTP metadata was supplied, preserving current behavior while preferring metadata.
- HIR string maps and `TemplateArgRef::debug_text` authority remain untouched by this packet.

## Proof

Ran the delegated proof exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_cxx_') > test_after.log 2>&1
```

Result: passed. The build completed and the delegated CTest regex matched `frontend_cxx_preprocessor_tests`, which passed. Proof log: `test_after.log`.

Supplemental supervisor validation after the slice:

```bash
ctest --test-dir build -j --output-on-failure -R '^cpp_'
```

Result: passed, 1147/1147. The regression checker on the narrow before/after logs found 0 new failures; it exited nonzero only because the pass count did not strictly increase.
