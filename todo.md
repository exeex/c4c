Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Completed Step 4 regression repair after owner-key canonicalization: qualified namespace functions are no longer skipped as out-of-class struct methods, and out-of-class method lookup now canonicalizes parser-owned owner/method `TextId`s through spelling carriers before consulting structured method maps.

- Rebuilt `make_out_of_class_struct_method_lookup_key` owner qualifiers, owner base, and method text id through `Module::link_name_texts` when spelling carriers are available.
- Kept structured method authority intact: a real owner-key hit still suppresses free-function lowering, while namespace-qualified free functions with no struct-method owner are lowered normally.
- Repaired the observed failures where namespace functions such as `api::bump`/`geo::dot` were not materialized and an out-of-class constructor body was not attached.

## Suggested Next

Next packet: let the supervisor decide whether this regression repair needs reviewer scrutiny because the smallest fix touched `src/frontend/hir/hir_build.cpp`, which was outside the packet's initial owned-file list but is the build-owned site of the failing owner-key consumer.

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
- This slice does not canonicalize member typedef `tag_text_id` itself; the touched tests keep member ids canonical and isolate owner-key behavior.
- Keep function/global namespace metadata changes out of the next packet unless the owner-key helper naturally supports them without changing lookup behavior.
- `make_out_of_class_struct_method_lookup_key` lives in `hir_build.cpp`; the initial owned-file list named `hir_lowering_core.cpp`, but that draft is not the active consumer for this regression.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_positive_sema_namespace_struct_runtime_cpp|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_hir_module_decl_lookup_structured_mirror') > test_after.log 2>&1`

Proof log: `test_after.log`.
