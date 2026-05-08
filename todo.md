Status: Active
Source Idea Path: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Retire Compatibility Authority

# Current Packet

## Just Finished

Completed Step 6 inventory-only pass for `Validate And Retire Compatibility Authority`.

Inventory classification for remaining rendered-owner compatibility routes touched by idea 151:

- Retired: local special-member classification in `src/frontend/parser/impl/declarations.cpp` no longer compares rendered owner strings. `probe_special_member_owner` and `consume_special_member_owner` use component `TextId` equality for constructor terminal matching; `qualified_owner` is not the local constructor/operator decision key.
- Retired: HIR out-of-class method attachment now prefers `make_out_of_class_struct_method_lookup_key` and skips rendered fallback after complete structured metadata in both `attach_out_of_class_struct_method_defs` and `lower_non_method_functions_and_globals`.
- Retired: struct-definition owner registration no longer depends on stale `qualifier_segments` alone when complete `qualifier_text_ids` are present; `make_ast_node_ns_qual_for_owner_key` and `make_ast_node_unqualified_text_id_for_owner_key` canonicalize AST spelling carriers into `Module::link_name_texts` before owner-key indexing.
- Retained display/diagnostic: parser `qualified_owner`, `qualified_op_name`, `qualified_ctor_name`, `Node::name`, `unqualified_name`, and `qualifier_segments` remain emitted spelling mirrors for AST names, dumps, diagnostics, and legacy function-name text. They are acceptable while structured owner metadata remains attached beside them.
- Retained syntax/display: generic declarator checks that test whether `decl_name` contains `::` before parsing method cv/ref qualifiers are syntax-shape gates, not semantic owner lookup. Keep them out of this idea unless they start feeding owner identity.
- Retained temporary compatibility: parser template-scope relabeling still looks up template owners through rendered `qualified_owner` / `qualified_owner_tag` text IDs in `declarations.cpp`. Owner: parser template scope recovery. Limitation: this still depends on rendered owner spelling for `FreeFunctionTemplate` to `EnclosingClass` relabeling. Removal condition: teach this path to use `QualifiedNameRef` / owner segment `TextId` keys or a structured template-owner lookup.
- Retained temporary compatibility: HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name` as the fallback method route. Owner: HIR lowerer. Limitation: only allowed when structured out-of-class metadata is absent or incomplete. Removal condition: all parser/HIR function producers for out-of-class methods carry complete `qualifier_text_ids`, `unqualified_text_id`, namespace/global metadata, and owner-indexed method registration.
- Retained temporary compatibility: HIR AST-node owner-key helpers may intern `qualifier_segments`, `unqualified_name`, or `name` spelling to repair parser-table/link-name-table id mismatches. Owner: HIR owner-key bridge. Limitation: spelling is a cross-table carrier, not semantic authority. Removal condition: parser owner carriers are stored in the same semantic text table or passed as already-canonical HIR owner keys.
- Retained temporary compatibility: TypeSpec aggregate owner lookup in `src/codegen/shared/llvm_helpers.hpp`, `src/frontend/hir/hir_types.cpp`, and LIR lowering still falls back from structured owner keys to rendered compatibility tags or final spellings for layout/type-name recovery. Owner: HIR/LIR aggregate layout bridge. Limitation: needed for legacy TypeSpec producers, parser-owned `tag_text_id`s, and missing `struct_def_order`/layout metadata. Removal condition: all aggregate TypeSpecs carry complete module-canonical owner keys or record defs and all layout declarations are owner-indexed.
- Retained temporary compatibility: member-typedef and signature type recovery in `src/frontend/hir/hir_functions.cpp` still builds `member_typedef_compatibility_name` from qualified text and splits it at `::` after structured owner-key attempts fail. Owner: HIR member typedef resolver. Limitation: only a fallback for incomplete TypeSpec owner metadata. Removal condition: deferred member typedef producers always carry `deferred_member_type_owner_key`, qualifier `TextId`s, and member `TextId`s.
- Retained temporary compatibility: LIR aggregate helpers such as `lookup_structured_layout_compatibility_decl`, `lookup_structured_name_id_by_owner_or_compatibility`, `llvm_aggregate_value_ty`, and member-access owner recovery still accept rendered compatibility tags after structured lookup misses. Owner: LIR lowering. Limitation: this is layout/name recovery, not parser owner probing. Removal condition: HIR module export gives LIR complete structured aggregate names and owner keys for every aggregate use.
- Out of scope/unrelated: generic parser `token_spelling(...)` calls for literals, keyword-like extensions, pragma/asm payloads, initializer member names, operator display names, and tests are not owner-identity authority for idea 151 unless they feed one of the routes above.

## Suggested Next

Next packet: retire the parser-local template-scope relabel compatibility in `src/frontend/parser/impl/declarations.cpp` by replacing rendered `qualified_owner` / `qualified_owner_tag` template-primary lookups with a structured owner lookup built from the existing `QualifiedNameRef` / owner segment `TextId` metadata; keep `qualified_owner` only as display and update focused parser coverage for a stale rendered owner spelling.

## Watchouts

- Do not treat `TextId` alone as semantic compound owner identity.
- Compound owner meaning still lives in owner segment sequences plus namespace/global qualification.
- Do not weaken out-of-class member, constructor, operator, nested-owner, or same-spelling ambiguity tests.
- Keep unrelated parser `token_spelling(...)` cleanup outside this idea unless it feeds owner identity.
- `qualified_owner` and `qualified_owner_tag` still feed template-owner lookup through rendered parser text IDs; this is the narrowest remaining parser-owned Step 6 retirement candidate.
- HIR `try_parse_qualified_struct_method_name` still splits rendered `Node::name`; it is now kept only after structured out-of-class metadata is absent or incomplete in both attached-method and non-method lowering.
- `make_ns_qual` still uses `qualifier_segments` strings to populate HIR `segment_text_ids` outside the AST owner-key helper; do not broaden into shared namespace/global metadata without a separate packet.
- The AST-node owner-key helpers now treat available spelling carriers as the cross-table canonicalization path for parser-owned ids; consumers without a reliable spelling carrier still need a separate route.
- The no-module `typespec_aggregate_owner_key(TypeSpec)` path remains a structured parser-id fallback because it has no link-name table for cross-table canonicalization.
- This slice does not canonicalize member typedef `tag_text_id` itself; the touched tests keep member ids canonical and isolate owner-key behavior.
- Keep function/global namespace metadata changes out of the next packet unless the owner-key helper naturally supports them without changing lookup behavior.
- `make_out_of_class_struct_method_lookup_key` lives in `hir_build.cpp`; the initial owned-file list named `hir_lowering_core.cpp`, but that draft is not the active consumer for this regression.
- `pr44164.c` still exposes frontend/HIR metadata drift: `struct Y` can exist in `struct_defs` without being listed in `struct_def_order`, and some member expressions carry stale rendered owner tags. This packet handles those facts in LIR consumers without parser changes.
- The const-init active-layout guard is a recursion safety valve; it should not be used to justify accepting future owner/layout cycles when exact metadata is available.
- The `member_symbol_id` recovery intentionally accepts only a unique layout hit.
- The stale rendered qualified-name test covers the non-method path by leaving the structured owner present but the structured method key absent; rendered method maps remain populated to prove the fallback is gated.
- Nearby same-feature behavior considered in Step 5: the new parser probe covers nested out-of-class methods reached from parsed owner-probe metadata, and the HIR probe covers stale same-suffix nested owner ambiguity at the consumer boundary. Constructors, destructors, and operators remain covered by existing owner-authority tests and should not be weakened while Step 6 inventories remaining compatibility routes.
- Do not retire HIR/LIR aggregate compatibility in the same packet as parser template-scope relabeling; that path is broader layout/type recovery and needs separate proof.

## Proof

Inventory-only packet; no implementation or tests changed and no build was required.

Validation run after editing `todo.md`: `git diff --check`.

Log state: no `test_after.log` update was made.
