Status: Active
Source Idea Path: ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retire Template Instantiated Record Rendered Keys

# Current Packet

## Just Finished

Step 3 implementation packet completed for template instantiated record
registration and lookup.

Completed work:

- Added structured template-instantiated record recovery helpers in
  `src/frontend/parser/impl/types/template.cpp` and
  `src/frontend/parser/impl/types/base.cpp` that compare the parser-owned
  template key plus structured argument keys against concrete records in
  `definition_state_.struct_defs`.
- `ensure_template_struct_instantiated_from_args` now preserves the injected
  parse path for missing rendered mirrors, but when a rendered mirror exists
  and the structured instantiation key is established, resolved `TypeSpec`
  metadata prefers the structured record over the rendered map payload.
- Direct template struct emission in `parse_base_type` now reuses an existing
  concrete instantiated record by structured instantiation key before falling
  back to `struct_tag_def_map`; the rendered map remains a compatibility mirror
  when no structured record carrier is available.
- Added focused stale rendered-map coverage in
  `tests/frontend/frontend_parser_tests.cpp` proving a poisoned
  `struct_tag_def_map` entry cannot override structured `Box<int>` direct
  emission identity.

## Suggested Next

Next coherent packet: Step 4 member typedef and nearby record carrier handoff
cleanup.

Suggested owned files:

- `src/frontend/parser/impl/types/base.cpp`
- `src/frontend/parser/impl/types/struct.cpp` only if record-member handoff
  registration needs a shared helper
- focused member typedef / record carrier tests

## Watchouts

- `ensure_template_struct_instantiated_from_args` must still run injected parse
  when the rendered mirror is missing; skipping that parse loses side effects
  needed by variable-template/member-typedef normalization.
- Rendered `struct_tag_def_map` writes remain compatibility mirrors for
  template instantiated records. They are still present, but no longer override
  structured direct-emission identity when the structured carrier exists.
- Qualified record definition setup still needs structured qualified-name
  metadata before the rendered set fallback can be retired there.

## Proof

Supervisor-selected proof command completed and wrote `test_after.log`:

```bash
bash -o pipefail -c "(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_hir_template_.*structured_metadata|cpp_hir_parser_type_base_.*structured_metadata|cpp_hir_member_typedef_.*structured_metadata|cpp_positive_sema_.*template.*|frontend_cxx_)$') | tee test_after.log"
```

Result: build succeeded; 234/234 selected tests passed.
