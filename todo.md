Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Started Step 4 consumer inventory for the parser owner handoff from `attach_out_of_class_method_owner` and `make_out_of_class_method_qn`.

- AST-backed lookup found `attach_out_of_class_method_owner` defined in `src/frontend/parser/impl/declarations.cpp` and called from `apply_function_decl_namespace` plus the special-member branches in `parse_top_level`; `make_out_of_class_method_qn` is called by `parse_top_level`.
- Parser handoff classification: `attach_out_of_class_method_owner` writes `namespace_context_id`, `is_global_qualified`, `unqualified_text_id`, `unqualified_name`, `qualifier_text_ids`, and `qualifier_segments` onto the function node. The `TextId` sequence is the structured carrier; copied segment strings are display/compatibility mirrors.
- Sema immediate consumer classification: `method_owner_key_from_qualifier` reaches `resolve_record_owner_key_from_qualifier` and `sema_record_owner_key_from_qualifier`, which derive owner identity from `Node::qualifier_text_ids`, `unqualified_text_id`, namespace context, and global qualification. This is structured carrier use, not rendered owner-string authority.
- HIR immediate consumer classification: `make_out_of_class_struct_method_lookup_key` and `attach_out_of_class_struct_method_defs` prefer `HirStructMethodLookupKey`/`HirRecordOwnerKey` owner metadata before falling back to `try_parse_qualified_struct_method_name`. The structured path is semantic authority; the rendered parse fallback is temporary compatibility for nodes without complete owner metadata.
- HIR route note: `make_ns_qual` currently rebuilds `NamespaceQualifier::segment_text_ids` by interning `Node::qualifier_segments` into the HIR link-name text table, so it consumes structured segment arrays but not the parser-owned `qualifier_text_ids` directly. Treat this as a compatibility bridge, not final structured identity authority.

## Suggested Next

Next packet: implement the first Step 4 HIR consumer tightening.

Owned files: `src/frontend/hir/hir_lowering_core.cpp`, `src/frontend/hir/hir_build.cpp`, `src/frontend/hir/hir_types.cpp`, `tests/frontend/frontend_hir_tests.cpp`, `todo.md`.

Packet boundary: make the out-of-class HIR attachment/skip path prove and prefer complete owner metadata from `Node::qualifier_text_ids`/owner keys, keep `try_parse_qualified_struct_method_name` only as an explicit compatibility fallback when structured metadata is absent or incomplete, and add/update focused HIR coverage that mutates rendered `Node::name`/`qualifier_segments` independently from owner metadata.

Suggested proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` splits rendered `Node::name`; leave it only as a clearly bounded compatibility fallback, never ahead of complete owner metadata.
- `make_ns_qual` currently uses `qualifier_segments` strings to populate HIR `segment_text_ids`; the next packet should avoid relying on stale rendered segment strings when parser metadata is available.
- No implementation files or tests were changed in this packet.

## Proof

Inventory-only Step 4 packet; no implementation change was made and no build was required.

No proof command was run, per delegated instructions for no-code inventory.

Log state: no `test_after.log` update was made.
