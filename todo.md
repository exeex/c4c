Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread Structured Owner Identity To Consumers

# Current Packet

## Just Finished

Completed Step 4 inventory of remaining HIR consumers that derive owner identity through `make_ns_qual` or rendered `qualifier_segments` mirrors.

- AST-backed symbol inventory confirmed `make_ns_qual` in `src/frontend/hir/hir_lowering_core.cpp`, `make_struct_def_node_owner_key`, `register_struct_def_node_owner`, `attach_out_of_class_struct_method_defs`, `lower_non_method_functions_and_globals`, and `lower_pending_struct_methods` in `src/frontend/hir/hir_build.cpp`, plus `lower_struct_method` call sites in `src/frontend/hir/impl/stmt/stmt.cpp` and `src/frontend/hir/impl/templates/struct_instantiation.cpp`.
- Struct-definition owner registration still has rendered-segment authority: `Lowerer::make_struct_def_node_owner_key` builds `HirRecordOwnerKey` from `make_ns_qual(sd, texts)`, and `Lowerer::lower_struct_def` stores `def.ns_qual` the same way before indexing `struct_owner_key`.
- Method lowering call sites do not need to rediscover owner identity from rendered method names: `lower_pending_struct_methods` and template instantiation paths pass an `owner_key` when available, and `lower_struct_method` stores that into `ctx.method_struct_owner_key`; its `fn.ns_qual = make_ns_qual(method_node, ...)` is function namespace metadata/link lookup, not the struct-owner binding authority.
- Remaining `make_ns_qual` uses in `lower_function`, consteval function lowering, globals, decl refs, builtin refs, and scalar-control refs are declaration namespace metadata or compatibility lookup, not the local out-of-class struct-member owner decision fixed by this route.
- `make_record_owner_key_for_type` and several TypeSpec-based paths already prefer `qualifier_text_ids`; their `qualifier_segments` fallback is temporary compatibility for absent/incomplete structured metadata.
- Text-table blocker: `Node::qualifier_text_ids` are parser-owned, while HIR owner indexes normally use `Module::link_name_texts`. Directly copying raw node ids into `HirRecordOwnerKey` is only safe when the module is known to share the parser text table; otherwise the implementation needs a helper that validates/canonicalizes each `TextId` through spelling into `link_name_texts`, matching the cross-table warning already documented in `codegen/shared/llvm_helpers.hpp`.

## Suggested Next

Next packet: add a small HIR helper for AST-node namespace qualifiers that prefers complete structured qualifier metadata but canonicalizes parser-owned `TextId`s into `Module::link_name_texts` before building HIR owner keys.

Suggested implementation boundary: update `make_struct_def_node_owner_key` and `lower_struct_def` to use the helper for struct-definition owner registration/indexing, then add focused HIR coverage where `Node::qualifier_segments` is stale but `qualifier_text_ids` plus spelling canonicalization still registers the correct owner.

Suggested proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_hir|frontend_parser_lookup_authority') > test_after.log 2>&1`

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; do not delete them until a structured lookup replacement exists.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; this slice keeps it only after structured out-of-class metadata is absent or incomplete, not after complete structured misses.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids`; this packet did not broaden into that shared helper because it has wider declaration/global/type-definition blast radius.
- Do not copy parser-owned `qualifier_text_ids` directly into `Module::link_name_texts` owner indexes unless the module has explicitly attached the same text table.
- Keep function/global namespace metadata changes out of the next packet unless the struct-definition helper naturally supports them without changing lookup behavior.

## Proof

Inventory-only slice; no build required and no `test_after.log` update made.

AST-backed queries used:
- `c4c-clang-tool-ccdb list-symbols` on `src/frontend/hir/hir_lowering_core.cpp`, `src/frontend/hir/hir_build.cpp`, and `src/frontend/hir/impl/stmt/stmt.cpp`.
- `c4c-clang-tool-ccdb find-definition` for `make_struct_def_node_owner_key` and `register_struct_def_node_owner`.
- `c4c-clang-tool-ccdb function-callers` / `function-callees` probes around `make_ns_qual`, `make_struct_def_node_owner_key`, and `lower_struct_method`; several method-scoped queries exceeded the tool's current matching limits, so exact classifications were completed with targeted text reads.
