Status: Active
Source Idea Path: ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit Parser Support Record Compatibility Bridges

# Current Packet

## Just Finished

Step 5 implementation packet completed for parser support record compatibility
bridges.

Completed work:

- Audited the `struct_tag_def_map` reads in parser support and the
  complete-object declaration check. Existing constant-layout and declaration
  paths already reject structured-metadata misses instead of completing through
  rendered maps.
- Tightened documentation in `resolve_record_type_spec` so retained map reads
  are explicitly secondary compatibility paths after direct `record_def`
  authority is absent.
- Extended `cpp_hir_parser_support_residual_metadata_test` coverage so a stale
  exact-context rendered map cannot override a complete structured
  `record_def`, while the remaining compatibility bridge is still limited to
  callers without the direct carrier.

## Suggested Next

Next coherent packet: Step 6 validation and remaining record compatibility
review.

Suggested owned files:

- `todo.md`
- any transient review artifact requested by the supervisor
- `test_after.log`

## Watchouts

- `ensure_template_struct_instantiated_from_args` must still run injected parse
  when the rendered mirror is missing; skipping that parse loses side effects
  needed by variable-template/member-typedef normalization.
- Rendered `struct_tag_def_map` writes remain compatibility mirrors for
  template instantiated records. They are still present, but no longer override
  structured direct-emission identity when the structured carrier exists.
- Member typedef lookup now depends on `TypeSpec::tpl_struct_origin_key` being
  preserved on concrete owner carriers; do not replace that with rendered
  `template_origin_name` recovery in later packets.
- Qualified record definition setup still needs structured qualified-name
  metadata before the rendered set fallback can be retired there.
- `resolve_record_type_spec` still has bounded map compatibility for
  non-layout parser callers that have TextId/context metadata but no direct
  `record_def`; Step 6 should decide whether that remaining bridge satisfies
  the source idea or needs a follow-up lifecycle route.

## Proof

Supervisor-selected proof command completed and wrote `test_after.log`:

```bash
bash -o pipefail -c "(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_hir_parser_support_.*structured_metadata|cpp_hir_parser_type_base_.*structured_metadata|cpp_hir_member_typedef_.*structured_metadata)$') | tee test_after.log"
```

Result: build succeeded; 17/17 selected tests passed.
