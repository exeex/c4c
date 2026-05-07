Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Binding Carriers

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Define Structured Binding Carriers", for parser deferred NTTP evaluation call sites with owner/index metadata.

- Added structured overloads for deferred NTTP defaults and captured template-argument token evaluation.
- Added a parser-side `ParserTemplateBindingSet` builder for metadata-aware NTTP bindings in `src/frontend/parser/impl/types/base.cpp`.
- Migrated metadata-aware deferred NTTP evaluation call sites in template argument and base-instantiation paths to pass `ParserTemplateBindingSet` directly instead of using legacy string-pair wrapper conversion.

## Suggested Next

Next coherent packet: migrate type-binding call sites that already have template owner/parameter metadata to structured `ParserTemplateBindingSet` carriers instead of spelling-only compatibility bindings.

## Watchouts

- Legacy parser wrapper conversion still keeps NTTP string-pair entries for no-metadata compatibility fallback, but those entries must not set structured authority.
- The new `base.cpp` builder marks only `ParserNttpBindingMetadata` entries as authoritative structured NTTP metadata; string-pair entries remain compatibility bindings.
- Type bindings in this packet still carry owner metadata but not parameter index metadata where the call site has only legacy type-pair data.
- HIR string maps and `TemplateArgRef::debug_text` authority remain untouched by this packet.

## Proof

Ran the delegated proof exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_lookup_authority_tests$') > test_after.log 2>&1
```

Result: passed. The build completed and `frontend_parser_lookup_authority_tests` passed. Proof log: `test_after.log`.

Supervisor-side supplemental validation after this slice:

```bash
ctest --test-dir build -j --output-on-failure -R '^cpp_'
```

Result: passed 1147/1147. The focused regression checker found 0 new failures; it exited nonzero only because the pass count did not strictly increase.
