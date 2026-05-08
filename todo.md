Status: Active
Source Idea Path: ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retire Template Instantiated Record Rendered Keys

# Current Packet

## Just Finished

Step 2 implementation packet completed for ordinary `parse_record_tag_setup`.

Completed work:

- Added a structured record lookup in `src/frontend/parser/impl/types/struct.cpp`
  based on current namespace context plus tag `TextId`.
- Non-body ordinary record tag setup now returns an existing structured parser
  record before consulting `struct_tag_def_map`; the rendered map remains a
  legacy complete-definition fallback when no structured record is found.
- Body record setup now uses structured complete-record identity for ordinary
  duplicate/shadow decisions before `defined_struct_tags`; rendered set fallback
  is limited to qualified legacy spellings where this packet does not yet carry
  structured qualified-name metadata.
- Added stale rendered map/set coverage in
  `tests/frontend/frontend_parser_tests.cpp` proving structured forward-record
  identity wins over stale `struct_tag_def_map` and stale `defined_struct_tags`
  does not force a shadow tag for a fresh structured definition.

## Suggested Next

Next coherent packet: demote the template instantiated record registration and
lookup path that still keys `struct_tag_def_map` / `defined_struct_tags` by
rendered mangled specialization names.

Suggested owned files:

- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/impl/types/base.cpp` if the instantiation helper route
  requires keeping the registration/lookup contract consistent
- focused template parser/HIR tests

## Watchouts

- This packet only covers ordinary unqualified parser record setup. Qualified
  record definition setup still needs structured qualified-name metadata before
  the rendered set fallback can be retired there.
- Template instantiated records still have semantic rendered-key authority at
  `template.cpp:735-766` and `base.cpp:8097-8105`; keep that as the next packet
  instead of mixing it with support/declaration fallbacks.
- `register_record_definition` still writes `struct_tag_def_map[source_tag]`
  and `[sd->name]` as compatibility mirrors after structured registration.

## Proof

Supervisor-selected proof command completed and wrote `test_after.log`:

```bash
bash -o pipefail -c "(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_hir_parser_core_.*structured_metadata|cpp_hir_parser_type_base_.*structured_metadata|cpp_hir_member_typedef_.*structured_metadata|cpp_hir_template_.*structured_metadata)$') | tee test_after.log"
```

Result: build succeeded; 26/26 selected tests passed.
