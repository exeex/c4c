# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Record Identity Through TypeSpec

## Just Finished

Completed Step 1 inventory for the typed record identity bridge. The selected
representation is a nullable parser-owned record definition pointer on
`TypeSpec`, provisionally `Node* record_def`, populated only for `TB_STRUCT` and
`TB_UNION` values when the parser already has the concrete `NK_STRUCT_DEF`.

This is stable enough for parser semantic lookup because record definition nodes
are arena-owned for the parser lifetime, are already the values stored in
`ParserDefinitionState::struct_defs` and `struct_tag_def_map`, and are the common
object passed through ordinary record parsing, explicit specialization reuse,
injected template instantiation, nested record fields, base classes, member
typedef lookup, layout, and const-eval support paths. It is intentionally not a
`TextId`: text identities are good spelling keys, but they cannot distinguish a
rendered compatibility spelling from the actual record object selected by
semantic parsing.

Inventory result:
- Direct record producers already have the pointer: `parse_struct_or_union`
  returns `Node* sd`, `register_record_definition` installs the same `sd`, and
  direct struct/union base-type parsing immediately builds the `TypeSpec`.
- Template producers already have the pointer: explicit full-specialization reuse
  sees `selected`, injected/direct instantiation creates `inst`, and template
  member lookup resolves through the instantiated struct node before falling back
  to rendered lookup.
- Nested/member producers often have the pointer in hand while constructing field
  declarations or walking owner nodes; copied `TypeSpec` values should preserve
  the pointer when substituting typedefs, bases, template args, fields, params,
  and member typedefs.
- Semantic consumers currently using `TypeSpec::tag` are layout and const-eval
  (`field_align`, `struct_align`, `struct_sizeof`, `compute_offsetof`,
  `eval_const_int`), incomplete object checks in declarations, `offsetof`
  folding, recursive member typedef/owner lookup in declarator parsing, template
  static member/base lookup, and template/direct-emission dedup compatibility
  checks.

Retained `TypeSpec::tag` categories:
- final spelling and emitted aggregate name for HIR/LIR/backend surfaces
- diagnostics, dumps, and user-visible type rendering
- namespace-qualified compatibility spelling for current maps and tests
- unresolved or tag-only forward declarations where no `NK_STRUCT_DEF` exists
- pending template spelling (`tpl_struct_origin`, mangled instance tag, and
  deferred member typedef text) until the instantiation has a concrete record node
- compatibility fallback for `struct_tag_def_map` while unconverted consumers
  remain

## Suggested Next

First bounded implementation packet: add the nullable `TypeSpec` record identity
payload and populate it only in direct parser-owned record construction paths.
Owned implementation should be limited to the `TypeSpec` definition plus
ordinary `parse_struct_or_union` / `register_record_definition` handoff sites
that already hold `Node* sd`; do not convert semantic consumers yet.

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain.

The first propagation packet should preserve behavior with null `record_def` and
must not force downstream HIR/LIR/backend consumers to understand parser node
pointers. Consumer conversion should follow in a separate packet by adding a
small parser-local helper that resolves `const TypeSpec&` to `Node*` through
`record_def` first and `struct_tag_def_map[ts.tag]` second.

## Proof

Inventory-only packet. Build/tests intentionally not run.

Focused proof command for the first implementation packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$'`
