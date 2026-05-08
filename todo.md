Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Completed Step 4 consumer-gap repair for `Thread Structured Owner Identity To Consumers`.

- Made AST-node owner-key helpers intern spelling carriers into `Module::link_name_texts` when building owner keys, including first-use parser-id collision cases where the real spelling was not already present in the module text table.
- Gated `lower_non_method_functions_and_globals` rendered `try_parse_qualified_struct_method_name` fallback behind incomplete structured out-of-class method metadata, matching `attach_out_of_class_struct_method_defs`.
- Added focused HIR lookup coverage for first-use owner-key parser-id collisions and for a complete structured method miss whose stale rendered qualified name would previously suppress ordinary function lowering.

## Suggested Next

Next packet: supervisor should decide whether Step 4 is ready for broader validation or another reviewer pass before selecting more consumer threading work.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; it is now kept only after structured out-of-class metadata is absent or incomplete in both attached-method and non-method lowering.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids`; this packet did not broaden into that shared helper because it has wider declaration/global/type-definition blast radius.
- The AST-node owner-key helpers now treat available spelling carriers as the cross-table canonicalization path for parser-owned ids; consumers without a reliable spelling carrier still need a separate route.
- The no-module `typespec_aggregate_owner_key(TypeSpec)` path remains a structured parser-id fallback because it has no link-name table for cross-table canonicalization.
- This slice does not canonicalize member typedef `tag_text_id` itself; the touched tests keep member ids canonical and isolate owner-key behavior.
- Keep function/global namespace metadata changes out of the next packet unless the owner-key helper naturally supports them without changing lookup behavior.
- `make_out_of_class_struct_method_lookup_key` lives in `hir_build.cpp`; the initial owned-file list named `hir_lowering_core.cpp`, but that draft is not the active consumer for this regression.
- `pr44164.c` still exposes frontend/HIR metadata drift: `struct Y` can exist in `struct_defs` without being listed in `struct_def_order`, and some member expressions carry stale rendered owner tags. This packet handles those facts in LIR consumers without parser changes.
- The const-init active-layout guard is a recursion safety valve; it should not be used to justify accepting future owner/layout cycles when exact metadata is available.
- The `member_symbol_id` recovery intentionally accepts only a unique layout hit.
- The stale rendered qualified-name test covers the non-method path by leaving the structured owner present but the structured method key absent; rendered method maps remain populated to prove the fallback is gated.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

Proof log: `test_after.log`.
