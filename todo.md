# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.2
Current Step Title: Prove Direct Record Member-Typedef Lookup

## Just Finished

Step 2.4.1 inventory-only packet completed. Live writers of the remaining
`register_struct_member_typedef_binding(owner, member, type)` mirror are:

- `src/frontend/parser/impl/types/struct.cpp:763`: record-body `using Alias =
  ...;` writer. Strongest carrier is the in-flight record body member typedef
  arrays (`member_typedef_names` / `member_typedef_types`), later copied to the
  owning `Node` at `src/frontend/parser/impl/types/struct.cpp:2265`. Missing
  carrier: no direct structured owner key is passed; it uses
  `current_struct_tag_text()`.
- `src/frontend/parser/impl/types/struct.cpp:803`: record-body `typedef ...`
  writer. Strongest carrier is the same in-flight member typedef arrays and
  final record `Node` arrays. Missing carrier: no direct structured owner key;
  it also uses `current_struct_tag_text()`.
- `src/frontend/parser/impl/types/base.cpp:3223`: template-instantiation clone
  writer. Strongest carrier is the instantiated record `Node` plus its cloned
  member typedef arrays and substituted `TypeSpec` values. Missing carrier: the
  mirror owner is the rendered instantiation name `mangled`, not a
  `QualifiedNameKey`.

Live readers that can observe that mirror are:

- `src/frontend/parser/impl/core.cpp:660`: `find_typedef_type(TextId)` detects
  rendered names containing `::`, builds a key with `known_fn_name_key(0,
  name_text_id)`, then reads `struct_typedefs`. Strongest carrier at this call
  boundary is only a rendered `TextId`; structured owner/member metadata is
  missing.
- `src/frontend/parser/impl/core.cpp:679` and `:690`: direct
  `find_typedef_type(QualifiedNameKey)` / `find_structured_typedef_type`
  readers. Strongest carrier is `QualifiedNameKey`.
- `src/frontend/parser/impl/core.cpp:769-794`: `find_visible_typedef_type`
  first does ordinary visible lookup, then reads `struct_typedefs` through
  `VisibleNameResult.key`. Strongest carrier is `QualifiedNameKey` plus
  namespace context id from visible-name resolution.
- `src/frontend/parser/impl/core.cpp:2278-2281`: current-record sibling
  fallback builds a key from rendered sibling text and calls
  `find_typedef_type(sibling_text_id)`. Strongest carrier is current namespace
  context plus rendered spelling; owner/member metadata is missing.
- `src/frontend/parser/impl/core.cpp:2316-2322`: using-alias type resolution
  checks `find_structured_typedef_type(target_key)` before textual fallback.
  Strongest carrier is `QualifiedNameKey`.
- `src/frontend/parser/impl/core.cpp:2816-2828`: `lookup_type_in_context`
  constructs `struct_typedef_key_in_context(context_id, name_text_id)` and
  reads `struct_typedefs`. Strongest carrier is namespace context id plus
  `QualifiedNameKey`.
- `src/frontend/parser/impl/types/types_helpers.hpp:153`, `:160`, `:212`,
  `:234`, and `:709`: qualified-type helper/probe readers use
  `resolve_qualified_type(qn).key` and `find_structured_typedef_type`. Strongest
  carrier is `QualifiedNameRef` lowered to `QualifiedNameKey`, with direct
  record-definition probes available through
  `qualified_record_definition_in_context`.
- `src/frontend/parser/impl/expressions.cpp:1394-1460`: qualified expression
  disambiguation first checks `qualified_type_record_definition_from_structured_authority`
  and scans the owner `Node` member typedef arrays; its fallback reparses the
  qualified spelling through `parse_base_type`. Strongest carrier for a first
  conversion is the owner record `Node` plus member typedef arrays; the fallback
  path is missing structured metadata and should not grow a rendered
  qualified-text helper.
- `src/frontend/parser/impl/types/declarator.cpp:760-789`: dependent typename
  resolution follows nested owners and scans `Node` member typedef arrays, then
  caches the resolved member typedef under the dependent spelling. Strongest
  carrier is direct record/declaration metadata plus member typedef arrays.
- `src/frontend/parser/impl/types/base.cpp:766-990` and later recursive call
  sites: base-type member typedef lookup resolves from `TypeSpec::record_def`
  when present, otherwise tries structured record/tag probes, then scans `Node`
  member typedef arrays and bases. Strongest carrier is `TypeSpec::record_def`
  plus member typedef arrays; fallback tag lookup by spelling is the remaining
  weak carrier.
- `src/frontend/hir/impl/templates/type_resolution.cpp:33-49`,
  `:83-345`, and `:351-520`: HIR member-typedef resolution reads AST `Node`
  member typedef arrays through `TypeSpec::record_def`, `struct_def_nodes_`,
  template origin metadata, and selected template pattern metadata. It does not
  call the parser mirror directly. Strongest carrier is `TypeSpec::record_def`
  / direct record `Node`; tag-string fallback remains only after record/origin
  metadata is absent.

## Suggested Next

Bounded executor packet for Step 2.4.2: convert only the two record-body
member-typedef writers in `src/frontend/parser/impl/types/struct.cpp` away
from the rendered `owner::member` mirror by carrying direct record/member
metadata through record finalization and relying on the existing
`member_typedef_names` / `member_typedef_types` arrays as the semantic source
for direct record member-typedef lookup. Add or keep one focused parser proof
that direct record/member metadata wins over stale rendered mirror authority.

Keep the template-instantiation writer in
`src/frontend/parser/impl/types/base.cpp:3223`, the remaining qualified/textual
readers, and broad mirror deletion out of this packet unless a required
record-finalization carrier is already available without widening the slice.

## Watchouts

Do not introduce a helper that accepts rendered `owner::member`, `std::string`,
or `std::string_view` qualified text and splits or reparses it into owner/member
identity. The only safe first conversion path is one that starts from an
already-structured carrier: record `Node`, member typedef arrays,
`QualifiedNameKey`, namespace context id, or `TypeSpec::record_def`.

The riskiest live reader is still `find_typedef_type(TextId)` for names
containing `::` because it has only rendered spelling at its boundary. Treat it
as a blocker/last consumer unless an upstream caller can pass a
`QualifiedNameKey` or record `Node` instead.

The HIR side already prefers `TypeSpec::record_def` and member typedef arrays;
avoid moving parser rendered-text reconstruction into HIR to compensate for
parser-side mirror deletion.

## Proof

Inventory-only; no build required and no `test_after.log` was changed.

Evidence commands run:

- `rg -n "register_struct_member_typedef_binding|struct_member_typedef|member_typedef|find_.*typedef|typedef.*member" src include tests -g'*.{cpp,hpp,h,cc,cxx}'` (the missing `include/` path made `rg` exit 2, but it still printed source/test matches)
- `rg -n "TypeSpec::record_def|record_def|member_typedef|typedefs|QualifiedNameKey|namespace_context|namespace.*id|context id|current_namespace" src include -g'*.{cpp,hpp,h,cc,cxx}'` (same missing `include/` path behavior)
- `rg -n "find_structured_typedef_type|find_typedef_type\\(|struct_typedefs|register_structured_typedef_binding|register_struct_member_typedef_binding|struct_typedef_key_in_context|make_struct_member_typedef_key" src/frontend -g'*.{cpp,hpp,h}'`
- targeted `sed -n` reads for the writer/reader ranges listed above
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/frontend/parser/impl/core.cpp find_structured_typedef_type build/compile_commands.json` and the matching `register_struct_member_typedef_binding` query were attempted; both reported that `core.cpp` was not directly loadable from the compile database, so the inventory used `rg` plus targeted source reads.
