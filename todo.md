# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Continued Step 3.3 by wiring parsed record-field declarations to carry
`unqualified_name` and `unqualified_text_id` from the declarator parser into
Sema. Regular fields, multi-declarator fields, function-pointer fields,
record enum fields, and nested-record declarator fields now provide member
`TextId` metadata on the parsed `NK_DECL` field nodes.

Deleted the Sema `struct_textless_field_names_by_key_` compatibility index and
its textless field-name lookup route. Instance-field lookup now succeeds for
the covered parsed path through structured field `TextId` metadata rather than
rendered field-name recovery.

Added focused parsed-source coverage in
`frontend_parser_lookup_authority_tests` proving parsed record fields carry
member `TextId`s and that implicit method-body field lookup validates without
the textless Sema compatibility path.

## Suggested Next

Continue Step 3.3 by reviewing the remaining Sema `lookup_symbol()` rendered
compatibility routes, especially enum constants and local/global cases where
the reference still lacks complete qualifier or namespace producer metadata.

## Watchouts

- The parsed record-field producer gap covered by this packet is closed; no
  remaining parsed-source field declaration in the exercised regular, enum,
  nested-record, multi-declarator, or function-pointer paths needs the deleted
  textless Sema field-name recovery.
- `c4c-clang-tools` was not needed for this packet; targeted `rg` and source
  reads were enough to find the existing declarator `out_name_text_id` channel.
- The static-member template-instantiation optimistic route noted in the prior
  packet remains unchanged.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 884/884
passing delegated positive Sema tests.

Additional check: `cmake --build --preset default --target frontend_parser_lookup_authority_tests && build/tests/frontend/frontend_parser_lookup_authority_tests`

Result: passed, including the new parsed record-field member `TextId` coverage
and the existing stale instance-field member lookup authority coverage.
