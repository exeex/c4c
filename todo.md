Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Binding Carriers

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Define Structured Binding Carriers", for the parser lookup-authority regression repair.

- Added an explicit parser carrier authority bit so legacy string-pair NTTP wrapper bindings remain compatibility bindings instead of becoming authoritative structured metadata.
- Updated legacy-to-structured NTTP conversion to mark only `ParserNttpBindingMetadata`-origin entries as authoritative structured metadata.
- Tightened deferred NTTP default lookup so metadata-origin bindings with a `TextId` match by `TextId` only, preventing rendered-name lookup from reopening after authoritative structured metadata misses.

## Suggested Next

Next coherent packet: migrate parser deferred evaluation call sites that can already supply owner/index metadata to pass `ParserTemplateBindingSet` directly instead of relying on the legacy string-pair wrapper conversion.

## Watchouts

- Legacy parser wrapper conversion still keeps NTTP string-pair entries for no-metadata compatibility fallback, but those entries must not set structured authority.
- Metadata-origin NTTP entries with `TextId` authority now intentionally do not fall back to rendered spelling when the `TextId` misses.
- Type-binding call sites still need a direct structured path in a later packet to avoid relying on spelling-only compatibility fields.
- HIR string maps and `TemplateArgRef::debug_text` authority remain untouched by this packet.

## Proof

Ran the delegated proof exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_lookup_authority_tests$') > test_after.log 2>&1
```

Result: passed. The build completed and `frontend_parser_lookup_authority_tests` passed. Proof log: `test_after.log`.
