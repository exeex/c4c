Status: Active
Source Idea Path: ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Sema Owner and Static-Member String Authority

# Current Packet

## Just Finished

Plan Step 1 `Classify Sema Owner and Static-Member String Authority` completed
as a classification-only packet.

Classification map:

- `resolve_owner_in_namespace_context(owner, namespace_context_id)`:
  available structured carriers are record declaration
  `Node::namespace_context_id`, record declaration `Node::unqualified_text_id`,
  record qualifier `TextId` path, `sema_symbol_name_key(record)`,
  `structured_record_keys_by_tag_`, and `struct_defs_by_unqualified_name_`.
  Current semantic authority is rendered-name driven: it indexes records by
  `unqualified_name(owner)`, filters by `candidate->namespace_context_id`, then
  accepts qualified owners by suffix comparison against rendered
  `candidate->name`. It returns rendered `candidate->name`. Classification:
  `owner`, `unqualified_name(owner)`, and `candidate->name` are semantic string
  authority today; `namespace_context_id` is structured authority but only as a
  filter.

- `enclosing_method_owner_struct(fn)`:
  available structured carriers are method `Node::namespace_context_id`,
  method `Node::unqualified_text_id`, method qualifier `TextId` path,
  `method_owner_records_[fn]` for in-class methods, direct owner record `Node*`,
  owner record `sema_symbol_name_key`, and `structured_record_keys_by_tag_`.
  Current out-of-class method authority first uses
  `qualified_method_owner_struct(fn)`, which reparses rendered `fn->name` at the
  last `::`, then calls `resolve_owner_in_namespace_context`; fallback checks
  `complete_structs_` / `complete_unions_` by rendered owner string and returns
  that string. In-class method authority uses direct `method_owner_records_[fn]`
  but still returns rendered `record->name`. Classification: out-of-class
  owner discovery from `fn->name` and rendered owner-set checks are semantic
  string authority; the in-class `Node*` link is structured authority but is
  converted back to display spelling for downstream state.

- `lookup_struct_static_member_type(tag, member, reference)`:
  available structured carriers are owner record
  `SemaStructuredNameKey` from `structured_record_key_for_tag(tag)`, member
  `reference->unqualified_text_id`, `struct_static_member_types_by_key_`, and
  `struct_base_keys_by_key_`; legacy carriers are rendered `tag`, rendered
  `member`, `struct_static_member_types_`, and `struct_base_tags_`. Current
  code computes the legacy result first, optionally computes the structured
  result for comparison, then returns the legacy result. Classification:
  `tag`/`member` and legacy base recursion are semantic string authority;
  by-key lookup is comparison-only today, not authority.

- Unqualified lookup fallback paths that inspect AST spelling:
  available structured carriers are `sema_local_name_key(reference)` from
  `reference->unqualified_text_id`, `sema_symbol_name_key(reference)` from
  `namespace_context_id` plus `unqualified_text_id`, structured local/global/
  function/overload/enum maps, and the parser-provided
  `Node::unqualified_name`. `lookup_symbol(name, reference)` already prefers a
  structured local match when `sema_local_name_key(reference)` resolves, but
  global/function/enum lookup still returns rendered legacy results after
  comparison. The `NK_VAR` fallback from rendered `n->name` to
  `n->unqualified_name` is an explicit compatibility fallback for parser
  namespace canonicalization of truly unqualified ids; it uses structured
  lookup when the second call carries the same reference. Implicit method field
  lookup calls `has_struct_instance_field(current_method_struct_tag_, n->name,
  n->unqualified_text_id)`, which compares structured member identity but
  returns the legacy rendered result. Classification: local structured lookup
  is structured-primary; global/function/enum returns, `n->unqualified_name`
  fallback, and implicit field `n->name`/legacy return are fallback or semantic
  string authority depending on whether structured lookup is absent or merely
  comparison-only.

## Suggested Next

First bounded implementation packet: Step 4, make
`lookup_struct_static_member_type` structured-primary. Change it to return the
`struct_static_member_types_by_key_` result when `reference->unqualified_text_id`
and a non-ambiguous owner key are available, retain rendered `tag/member`
lookup only as a named compatibility fallback when structured identity is
absent, and add focused static-member tests covering colliding member names
under distinct owners or namespace contexts.

## Watchouts

`resolve_owner_in_namespace_context` and `enclosing_method_owner_struct` are
larger owner-routing changes because they currently return rendered owner
strings that feed `current_method_struct_tag_` and downstream legacy maps. The
static-member helper is the smallest first packet because its by-key map and
member `TextId` plumbing already exist.

`qualified_method_owner_struct(fn)` reparses `fn->name`; do not expand that
pattern. Parser carriers to prefer in later owner packets are
`Node::namespace_context_id`, `Node::unqualified_text_id`,
`Node::qualifier_text_ids`, and direct `method_owner_records_` owner nodes.

## Proof

No build/test required by the delegated classification-only proof. Evidence was
gathered with targeted `c4c-clang-tool-ccdb` definition/signature queries plus
targeted `rg`/source windows over `src/frontend/sema/validate.cpp` and parser
name-carrier definitions. `test_after.log` was not touched per packet
constraints. `git diff --check -- todo.md` passed.
