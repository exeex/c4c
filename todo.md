Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Completed Step 4 TypeSpec/HIR owner-key consumer slice: record-def-backed TypeSpec owner-key builders now prefer the AST-node owner-key helper path when a `Module::link_name_texts` table is available, so parser-owned qualifier/base ids canonicalize through spelling carriers before structured lookup.

- Updated `make_record_owner_key_for_type` for layout/HIR TypeSpec lookup.
- Updated `record_owner_key_from_type_metadata` for callable zero-sized return lookup.
- Updated the `Module`-aware `typespec_aggregate_owner_key` path in the shared LLVM helper while keeping the no-module direct TypeSpec fallback structured and non-rendered.
- Added focused `frontend_hir_lookup_tests` coverage for parser-owned qualifier/base TextId collisions canonicalizing to link-name TextIds across the touched layout, callable-return, and shared helper consumers.

## Suggested Next

Next packet: inspect the remaining direct TypeSpec qualifier-id consumers in member-typedef/template-owner lookup and choose one narrow path where a spelling carrier exists for the same helper treatment.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; this slice keeps it only after structured out-of-class metadata is absent or incomplete, not after complete structured misses.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids`; this packet did not broaden into that shared helper because it has wider declaration/global/type-definition blast radius.
- The helper can only repair parser-owned ids when the link-name table already has or can intern the spelling carrier; consumers without a reliable spelling carrier still need a separate route.
- The no-module `typespec_aggregate_owner_key(TypeSpec)` path remains a structured parser-id fallback because it has no link-name table for cross-table canonicalization.
- Keep function/global namespace metadata changes out of the next packet unless the owner-key helper naturally supports them without changing lookup behavior.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

Proof log: `test_after.log`.
