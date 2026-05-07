Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Binding Carriers

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Define Structured Binding Carriers", for parser type-binding call sites with owner/index metadata.

- Added `ParserTypeBindingMetadata` as the parser/Sema-owned structured carrier for template type bindings.
- Updated the parser-side `ParserTemplateBindingSet` builder to overlay authoritative owner/index type metadata while leaving spelling-pair type bindings as compatibility entries.
- Migrated preliminary template-argument and base-instantiation default-evaluation paths to pass structured type metadata into `ParserTemplateBindingSet` where the owner template and parameter index are known.
- Kept legacy type-pair conversion compatibility-only in `template.cpp`; structured type matching now prefers authoritative metadata when a binding set contains it.

## Suggested Next

Next coherent packet: audit remaining parser template-binding consumers outside deferred NTTP evaluation and decide whether they should stay compatibility-only or get structured owner/index carriers in a separate bounded slice.

## Watchouts

- Legacy parser wrapper conversion still keeps string-pair type and NTTP entries for no-metadata compatibility fallback, but those entries must not set structured authority.
- `ParserTemplateBindingSet::has_structured_type_metadata` is now driven by authoritative type metadata, not by the presence of an owner key on legacy spelling pairs.
- The migrated paths only mark type bindings authoritative where the call site has owner template plus parameter index metadata.
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

Result: passed 1147/1147.
