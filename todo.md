Status: Active
Source Idea Path: ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Member Typedef And Record Carrier Handoffs

# Current Packet

## Just Finished

Step 4 implementation packet completed for member typedef and nearby record
carrier handoffs.

Completed work:

- Member-typedef sidecar lookup in `src/frontend/parser/impl/types/base.cpp`
  now prefers a `TypeSpec` owner carrier's structured `tpl_struct_origin_key`
  when building the member typedef key and when finding the owner primary for
  template-argument substitution.
- The rendered `Node::template_origin_name` path remains a secondary
  compatibility fallback for owners that do not have a structured
  `TypeSpec` carrier.
- Extended focused member-typedef coverage in
  `tests/frontend/frontend_parser_lookup_authority_tests.cpp` so stale
  rendered owner spelling cannot override the structured owner key for both
  NTTP and type-argument member typedef substitution.

## Suggested Next

Next coherent packet: Step 5 parser support record compatibility bridge audit.

Suggested owned files:

- parser support helpers that still receive `struct_tag_def_map`
- focused parser support / record compatibility tests
- `todo.md`

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

## Proof

Supervisor-selected proof command completed and wrote `test_after.log`:

```bash
bash -o pipefail -c "(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_hir_template_.*structured_metadata|cpp_hir_parser_type_base_.*structured_metadata|cpp_hir_member_typedef_.*structured_metadata|cpp_positive_sema_.*template.*|frontend_cxx_)$') | tee test_after.log"
```

Result: build succeeded; 234/234 selected tests passed.
