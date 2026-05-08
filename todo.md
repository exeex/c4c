Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Canonicalize HIR Owner-Key Carriers

# Current Packet

## Just Finished

Completed Plan Step 3 by canonicalizing the focused HIR struct-definition owner-key bridge.

`make_struct_def_node_owner_key` now reuses complete `struct_def_owner_by_node_` identity before rebuilding a key from rendered node spellings. Its immediate AST owner-key helpers now keep complete node-carried `TextId` metadata when it is present and agrees with any spelling carrier, using rendered spelling repair only for missing or stale/colliding IDs. Focused coverage now proves complete node-carried owner IDs beat stale rendered spellings, while existing collision tests continue to prove parser/module `TextId` collisions are repaired instead of copied as owner identity.

## Suggested Next

Next coherent packet: choose the next rendered-owner authority family from the Step 1 inventory outside the focused struct-definition owner-key bridge, unless supervisor review wants more coverage around this same helper path.

## Watchouts

- `make_ast_node_ns_qual_for_owner_key` and `make_ast_node_unqualified_text_id_for_owner_key` still use rendered spelling as a repair path when node `TextId` metadata is missing or mismatched; this packet only demotes rendered spelling from primary authority when complete structured identity is available.
- Existing parser-owned collision fixtures remain intentionally spelling-repaired because their raw IDs collide with unrelated `Module::link_name_texts` entries.
- Keep the next packet separate from parser owner-probe code, aggregate layout compatibility, member typedef recovery, and LIR lowering unless the supervisor explicitly selects one of those families.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target c4cll frontend_hir_lookup_tests && ctest --test-dir build -R '^frontend_hir_lookup_tests$' --output-on-failure > test_after.log`
