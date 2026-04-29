Status: Active
Source Idea Path: ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit LIR, BIR, And Backend Handoff Text Authority

# Current Packet

## Just Finished

Step 3 audited Sema and HIR text authority against existing follow-up ideas
135 and 136, including the Step 2 carryover helper consumers.

Structured identity:

- Sema structured mirrors exist for globals, functions, ref/cpp overloads,
  consteval functions, enum/global/local const values, local scopes, enum
  variants, record completeness, static members, instance fields, and bases:
  `SemaStructuredNameKey`, `TextId`, `Const*ByKey`, and `*_by_text` maps are
  structured carriers when the lookup result is returned from the structured
  path or used as a keyed consteval environment.
- HIR structured identity exists for record owners and lookup keys:
  `NamespaceQualifier`, declaration `TextId`, `ModuleDeclLookupKey`,
  `HirRecordOwnerKey`, `HirStructMethodLookupKey`,
  `HirStructMemberLookupKey`, `MemberSymbolId`, `LinkNameId`, and
  `CompileTimeRegistryKey` maps are the intended authority surfaces.
- HIR compile-time registries already provide structured-first helpers for
  declaration-keyed lookup: `find_template_def(Node*, rendered_name)`,
  `find_template_struct_def(Node*, rendered_name)`,
  `find_template_struct_specializations(Node*, rendered_name)`, and
  `find_consteval_def(Node*, rendered_name)` first probe structured maps and
  only fall back to rendered names when the caller supplies them.
- HIR local `FunctionCtx` maps for locals, params, labels, function-pointer
  signatures, static globals, block const bindings, and pack params are
  function-scope lowering state. They use parser spelling inside one lexical
  lowering context, not cross-IR owner/member/template authority.

Legitimate display or final text:

- Diagnostics and debug text remain legitimate: Sema diagnostic messages,
  compile-time/HIR diagnostic strings, HIR dump/printer text, debug parity
  counters, pending-template-type context names, and formatted unresolved
  template-type messages.
- Final/generated HIR text remains legitimate when not used as source identity:
  sanitized symbols, mangled template instance names, operator method spelling,
  emitted function/global names, link names, generated specialization keys,
  string literal payloads, asm text, and label names.
- HIR template parameter names and binding-map names are legitimate local
  template-substitution keys while they are scoped to a primary/template
  pattern and not used as cross-owner record/template lookup by themselves.

Compatibility fallback already covered by ideas 135 and 136:

- Idea 135 covers Sema rendered owner/static-member/member fallback authority:
  `resolve_owner_in_namespace_context()`, `enclosing_method_owner_struct()`,
  `lookup_struct_static_member_type()`, `has_struct_instance_field()`, and
  unqualified variable lookup fallback from `n->name` to
  `n->unqualified_name`. These paths compare structured mirrors but still
  return legacy string-map results in several cases.
- Idea 135 also covers Sema record-completeness and record-key lookup by
  rendered `TypeSpec::tag`: `structured_record_keys_by_tag_` is keyed by
  rendered tag and is used to reach structured completeness/base/static-member
  mirrors. That is part of the same owner/member lookup cleanup, not a separate
  Step 4 issue.
- Idea 136 covers HIR rendered owner/method/member authority:
  `try_parse_qualified_struct_method_name()`,
  `attach_out_of_class_struct_method_defs()`,
  `find_struct_method_mangled()`,
  `find_struct_method_link_name_id()`,
  `find_struct_method_return_type()`,
  `find_struct_static_member_decl()`,
  `find_struct_static_member_const_value()`, and
  `find_struct_member_symbol_id()` still derive or return semantic answers
  from rendered tags, rendered `tag::member` strings, or `MemberSymbolTable`
  rendered lookup while by-owner maps only check parity.
- Idea 136 covers HIR template struct primary/specialization authority:
  `find_template_struct_primary()`, `find_template_struct_specializations()`,
  `canonical_template_struct_primary()`, `seed_pending_template_type()`,
  `realize_template_struct_if_needed()`, and `recover_template_struct_identity_from_tag()`
  still depend on rendered `tpl_struct_origin`, rendered tags, or family-root
  parsing while structured owner maps only verify parity or provide fallback.
- Idea 136 covers HIR static-member/type-trait helper authority where rendered
  struct maps are still consumer-facing: `eval_struct_static_member_value_hir()`
  recurses through `struct_defs` by rendered tag and `member_name`.

Compatibility fallback outside the Sema/HIR follow-up scope:

- HIR compile-time state still has rendered-name overloads for template,
  template-struct, consteval, enum, and const-int maps. These are documented
  compatibility fallbacks when declaration keys are unavailable; the
  structured overloads are already primary when callers pass a node/key.
- Step 2 parser-support carryover: HIR uses `types_compatible_p()` with empty
  typedef maps in `normalize_zero_sized_struct_return_from_body()` and
  template pattern matching. With empty typedef maps, these uses compare
  `TypeSpec` payloads and do not add string-map authority beyond existing HIR
  template/type cleanup covered by idea 136.
- Step 2 parser-support carryover: HIR uses the string-compatible
  `eval_const_int()` overload for struct static member values with empty
  struct/const maps or NTTP binding maps. The empty-map uses are compatibility
  wrappers around expression folding; the NTTP binding-name use is local
  template substitution state. The rendered owner/member lookup around those
  values is already covered by idea 136.

Suspicious authority:

- No new Sema/HIR suspicious authority was found outside ideas 135 and 136.
  The suspicious Sema families all map to idea 135, and the suspicious HIR
  owner/member/method/template families all map to idea 136.
- The Step 2 parser known-function-name gap remained outside ideas 135 and 136:
  `has_known_fn_name(const std::string&)`,
  `register_known_fn_name(const std::string&)`, rendered registration call
  sites, and fallback parsing into `QualifiedNameKey` are now mapped to
  `ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md`.
  This keeps the parser binding/disambiguation cleanup separate from the
  Parser record/template, namespace visible-name, AST payload, Sema, and HIR
  follow-up ideas.

## Suggested Next

Proceed to Step 4 for LIR, BIR, and backend handoff text-authority audit.
Step 3 did not require new Sema/HIR follow-up ideas beyond 135 and 136, and the
Step 2 parser known-function-name compatibility spelling gap now has focused
open follow-up coverage in idea 137.

## Watchouts

Do not fold the known-function-name gap into ideas 135 or 136; it is covered by
idea 137 as a parser binding/disambiguation family from Step 2. Do not create
duplicate Sema/HIR ideas for owner/static-member/method/template rendered
lookup without first reconciling with ideas 135 and 136. Parser-support helper
consumers are not a new parser gap based on this audit; their HIR-side rendered
owner/template risks are already represented by idea 136.

## Proof

Not run per packet: no build proof required for read-only Sema/HIR
text-authority audit and `todo.md` update; tests were explicitly out of scope.
