# Template Arg Transport De-Stringify Todo

Status: Active
Source Idea: ideas/open/51_template_arg_transport_de_stringify_refactor.md
Source Plan: plan.md

## Active Item

- Step 4/5 follow-through after the field deletion slice: migrate remaining
  template-arg transport away from debug-text-heavy storage into fully typed
  per-arg payloads so deferred NTTP and HIR paths can stop reparsing internal
  spellings in the common case.
- Iteration focus: keep the tree green after deleting `tpl_struct_arg_refs`,
  then tighten the new `TemplateArgKind` transport so each arg is stored in a
  real `TemplateArgRefList` object instead of split parallel arrays.
- Hard acceptance rule remains: this plan is only fully done when the old field
  is gone and the new architecture no longer relies on string-prefix semantic
  recovery.
- Next intended slice: target `parser_types_template.cpp` directly so deferred
  builtin-trait/member-evaluation consumes structured template args earlier,
  instead of reparsing token text in the remaining common path.
- Next intended slice: continue on `parser_types_template.cpp`, especially the
  `Trait<args>::member` token-injection path, so instantiated lookup no longer
  has to re-spell complex type args back into ad hoc tokens in the common case.
- Next intended slice: replace the remaining injected-parse instantiation seam
  with a typed helper that can realize template structs without rebuilding a
  token stream, now that both parser call sites share one injection helper.
- Next intended slice: implement the inside of the new
  `Parser::ensure_template_struct_instantiated_from_args(...)` seam with a
  typed realization path so parser no longer has to rely on injected parse for
  common template instantiation cases.
- Next intended slice: move more of the concrete struct realization logic
  behind the new parser seam so the remaining injected-parse fallback becomes a
  small internal detail rather than the default path.
- Next intended slice: keep shrinking parser-side string-derived semantic
  fallback by routing more `text -> TypeSpec` recovery through the new shared
  decode seam, then convert another common instantiation case to a true typed
  fast path.
- Next intended slice: push one more common parser instantiation case onto a
  true typed fast path, now that most obvious parser-side `text -> TypeSpec`
  recovery sites have been centralized.
- Next intended slice: keep converting the remaining parser instantiation
  fallback inside `ensure_template_struct_instantiated_from_args(...)`, but the
  next highest-value work now likely sits in the still-stringy HIR helper
  paths.
- Next intended slice: trim the remaining HIR fallback glue itself, especially
  the `resolve_struct_member_typedef_type(...)` debug-ref fallback branch and
  the builtin-text decode helpers, now that the main callers already route
  through typed `materialize_template_args(...)`.
- Next intended slice: now that `materialize_template_args(...)` shares one
  HIR helper, keep trimming the helper's fallback seams, especially
  `parse_builtin_typespec_text(...)` decode and the remaining debug-ref-only
  recovery path for typed args that still lack concrete payload.
- Warning probe now marks HIR string-fallback helpers as deprecated. Current
  build output points at the remaining hot spots in
  `hir_templates.cpp`: two typed-to-string fallback calls inside the new
  materializer helper, plus the `resolve_struct_member_typedef_type(...)`
  fallback branch that still collects and reassigns debug refs.

## Completed

- Activated `ideas/open/51_template_arg_transport_de_stringify_refactor.md`
  into `plan.md`.
- Rewrote the idea into an execution-oriented runbook with an explicit hard
  acceptance criterion: `tpl_struct_arg_refs` must be completely deleted for the
  refactor to count as done.
- Added `TemplateArgKind` plus structured template-arg storage fields onto
  `TypeSpec` in [ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp).
- Deleted `TypeSpec::tpl_struct_arg_refs` and migrated all frontend
  reader/writer sites under `src/frontend/` to the new transport helpers.
- Updated parser/HIR pending-type identity and template rebuild helpers to use
  `encode_template_arg_debug_list(...)` instead of the removed field.
- Verified the field removal mechanically:
  `rg -n "tpl_struct_arg_refs" src/frontend` returns no matches.
- Rebuilt `c4cll` successfully after the migration.
- Re-ran focused regressions successfully:
  `cpp_positive_sema_template_builtin_is_enum_inherited_value_runtime_cpp`
  `cpp_positive_sema_template_builtin_is_enum_qualified_inherited_value_runtime_cpp`
  `cpp_positive_sema_eastl_inherited_trait_value_template_arg_parse_cpp`
  `cpp_eastl_type_traits_parse_recipe`
- Replaced the interim parallel-array storage on `TypeSpec` with a single
  `TemplateArgRefList`, so the transport shape now matches the refactor target
  more closely and new code can move toward reading per-arg `kind/type/value`.
- Updated parser alias/member preservation so nested `@origin:args` rebuild now
  reconstructs typed nested template args recursively and writes
  `TemplateArgRef.kind/type/value` instead of only storing rebuilt debug text.
- Updated deferred alias-member preservation paths to attach structured template
  args directly from `ParsedTemplateArg` vectors before handing off to later
  stages.
- Updated HIR nested `@origin:args` materialization to rebuild recursive typed
  `TemplateArgRef` payloads from `HirTemplateArg`, so HIR no longer needs to
  reattach nested pending args as debug-text-only lists in that common path.
- Updated HIR deferred NTTP expression env to recognize nested `@origin:args`
  references and rebuild them as typed `TemplateArgRef` payloads instead of
  treating those refs as opaque debug text.
- Updated parser deferred template-arg resolution so multi-token template args
  now go through shared `decode_type_tokens(...)` instead of a builtin-only
  text parser, letting qualified and other structured type spellings reach the
  common path as typed args earlier.
- Introduced a shared parser-side `TypeSpec -> token` helper and switched the
  deferred `Trait<args>::member` token-injection path to use it, replacing the
  old ad hoc subset emitter for injected type args.
- Introduced a shared parser-side `instantiate_template_struct_via_injected_parse(...)`
  helper and moved both template-base instantiation and deferred
  `Trait<args>::member` instantiation onto that single seam.
- Promoted parser-side specialization-selection + mangled-name +
  ensure-instantiated flow into `Parser::ensure_template_struct_instantiated_from_args(...)`,
  so both parser call sites now share one higher-level typed entry point before
  the remaining injected-parse fallback.
- Promoted parser-side mangled-name construction into
  `Parser::build_template_struct_mangled_name(...)`, so pack/default-aware
  instantiation naming is now shared between the main parser instantiation path
  and the new parser instantiation seam.
- Added the first true typed fast path inside
  `Parser::ensure_template_struct_instantiated_from_args(...)`: explicit full
  specializations can now register/use their existing concrete struct node
  directly without going through injected parse.
- Added `Parser::decode_type_ref_text(...)` and routed both deferred parser
  type-trait decoding and one major template-base rebuild path through that
  shared seam instead of open-coded `struct_/enum_/mangled` string recovery.
- Routed two additional parser common-path decode sites through
  `Parser::decode_type_ref_text(...)`, reducing direct open-coded
  `mangled/prefix/builtin` recovery inside deferred template arg parsing.
- Added a typed fast path for template-base rebuild: when pending base args are
  already present in `tpl_struct_args`, parser now reconstructs
  `ParsedTemplateArg` directly from structured payloads instead of first
  round-tripping through `arg_refs_str` text in that common case.
- Replaced the HIR pending-owner common path in
  [hir_expr.cpp](/workspaces/c4c/src/frontend/hir/hir_expr.cpp) and
  [hir_types.cpp](/workspaces/c4c/src/frontend/hir/hir_types.cpp): those sites
  now materialize typed `TemplateArgRefList` entries directly from AST template
  args instead of packing all args into a single `debug_text` blob before
  pending owner resolution.
- Refactored the `resolve_struct_member_typedef_type(...)` binding path in
  [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp) so
  template-owner/member-typedef substitution now rewrites structured
  `TemplateArgRef` entries directly in the common path and only falls back to
  debug-text reconstruction when typed payload is unavailable.
- Added a typed `materialize_template_args(...)` overload in
  [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp), and
  switched both `resolve_struct_member_typedef_if_ready(...)` and
  `realize_template_struct(...)` to use structured `TypeSpec::tpl_struct_args`
  directly instead of first collecting debug-ref strings in the common path.
- Refactored `materialize_template_args(...)` in
  [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp)
  behind a dedicated HIR helper so the string-backed and typed-backed entry
  points now share one core implementation for pack/default/binding materialization.
- Marked the main HIR string-fallback helpers as `[[deprecated]]` so build
  warnings now surface the remaining migration stragglers:
  string-backed `materialize_template_args(...)`,
  `collect_template_arg_debug_refs(...)`, and
  `assign_template_arg_debug_refs(...)`.
