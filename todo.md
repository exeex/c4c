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
- Current warning state after the latest slice: the typed-materializer fallback
  hits are gone, and the remaining deprecated call sites are now concentrated
  in the single `resolve_struct_member_typedef_type(...)` fallback branch in
  `hir_templates.cpp`.
- Current warning state after the latest slice: the HIR deprecated probe no
  longer reports any call sites for the marked string-fallback helpers during
  `c4cll` build, so remaining cleanup can focus on deleting or narrowing those
  helpers rather than hunting more users.
- HIR dead-helper cleanup: `collect_template_arg_debug_refs(...)` and
  `assign_template_arg_debug_refs(...)` are now fully removed from
  `src/frontend/hir/`.
- HIR interface cleanup: the external string-backed
  `materialize_template_args(...)` overload is now gone; string ref handling
  only survives as an internal fallback inside the HIR materializer helper.
- HIR typed-path cleanup: `resolve_explicit_typed_arg(...)` no longer falls back
  to parsing type-arg `debug_text`; typed type args now require structured
  payload and only value args still consult textual fallback.
- HIR type-ref decode cleanup has started: `decode_type_ref(...)` no longer
  runs separate top-level builtin/struct probes before suffix stripping; it now
  routes plain and suffixed type refs through one base-resolution path.
- HIR AST NTTP preservation: AST template value args now resolve deferred
  `$expr:...` references through `resolve_ast_template_value_arg(...)` instead
  of silently collapsing missing NTTP bindings to `0`, fixing
  `sizeof...(Ts)`-style inherited trait/value transport in common HIR paths.
- HIR inherited static-member stability: template-instantiated base tags are
  now realized more eagerly during struct instantiation, inherited `::type`
  lookups can realize owner structs before walking concrete bases, and concrete
  instantiated trait structs have a direct `::value` fallback so inherited
  static member lookup no longer depends entirely on base-chain metadata being
  perfect.
- HIR debug-helper cleanup has started in earnest: `hir_templates.cpp` no
  longer routes typed-materialization fallback or member-typedef binding
  rewrites through `template_arg_debug_text_at(...)`; those paths now read
  `TemplateArgRef` directly and only synthesize per-arg text locally when they
  genuinely need a compatibility string.
- Parser debug-helper cleanup has started too: `parser_types_base.cpp` no
  longer rebuilds transformed owner/template-arg text via the generic
  `encode_template_arg_debug_list(...)` helper in its common rebuild paths,
  and dependent-owner scanning now walks structured `TemplateArgRef` payloads
  recursively instead of first flattening them back to one debug string.
- Key/identity cleanup is now underway too: the HIR pending-type key path and
  parser canonical template-type key path no longer call the generic
  debug-list flattener directly; both now build their identity strings from
  structured `TemplateArgRef` payloads with key-specific local encoders.
- HIR nested-type ref emission is now on the same path: `encode_template_type_arg_ref_hir(...)`
  no longer routes nested arg emission through the generic debug-list helper
  and instead builds its compatibility string from structured `TemplateArgRef`
  entries directly.
- HIR object-call fallback now covers `integral_constant`-style trait objects:
  zero-arg `operator()` calls on struct objects with a known static `value`
  fold directly through the existing static-member const path, which keeps
  inherited trait object calls stable even when base method realization lags
  behind base-value transport.

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
- Removed the deprecated typed-to-string fallback usage inside the HIR
  materializer helper: `materialize_from_typed(...)` now handles the no-arg
  default case directly and only falls back by rebuilding refs locally, so the
  remaining deprecated hits are confined to the member-typedef fallback path.
- Replaced the last deprecated HIR helper usage in
  `resolve_struct_member_typedef_type(...)` with a local `TemplateArgRef`
  rebuild, so the current `c4cll` build no longer reports deprecated
  call sites for the marked string-fallback helpers.
- Deleted the now-unused HIR debug-ref helper pair from
  [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp):
  `collect_template_arg_debug_refs(...)` and
  `assign_template_arg_debug_refs(...)`.
- Deleted the now-unused public HIR string-backed
  `materialize_template_args(...)` overload from
  [hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)
  and [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp),
  so the string path is no longer a first-class Lowerer API.
- Removed the type-arg `debug_text` fallback from
  `resolve_explicit_typed_arg(...)` in
  [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp), so
  typed HIR template-arg materialization now relies on structured type payload
  for type args instead of reparsing their text form.
- Simplified `decode_type_ref(...)` in
  [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp) so
  it no longer duplicates the plain builtin/struct decode before the suffix
  handling pass; plain and suffixed refs now share one base-resolution path.
- Fixed HIR handling for deferred AST NTTP value args by adding
  `resolve_ast_template_value_arg(...)` and using it in template global
  instantiation, pending owner arg assignment, and template static-member const
  evaluation. This preserves `$expr:...` AST payloads such as `sizeof...(Ts)`
  instead of collapsing unresolved bindings to zero.
- Fixed inherited static-member lookup for instantiated trait structs by:
  realizing template bases more eagerly in `append_instantiated_template_struct_bases(...)`,
  allowing `resolve_struct_member_typedef_if_ready(...)` to realize owner
  structs before resolving inherited `::type`, preserving deferred value-arg
  debug text during HIR binding rewrites, and adding a concrete
  `try_eval_instantiated_struct_static_member_const(...)` fallback for
  instantiated trait-family `::value` lookups.
- Verified the focused regression set still passes:
  `cpp_positive_sema_inherited_static_member_lookup_runtime_cpp`
  `cpp_positive_sema_variadic_template_arg_sizeof_pack_parse_cpp`
  `cpp_positive_sema_template_builtin_is_enum_inherited_value_runtime_cpp`
  `cpp_positive_sema_template_builtin_is_enum_qualified_inherited_value_runtime_cpp`
  `cpp_positive_sema_eastl_inherited_trait_value_template_arg_parse_cpp`
  `cpp_eastl_type_traits_parse_recipe`
- Reworked `hir_templates.cpp` so typed HIR common paths stop consulting the
  generic `template_arg_debug_text_at(...)` helper: typed fallback refs and
  member-typedef binding rewrites now stringify directly from each
  `TemplateArgRef`, shrinking one more class of debug-text semantic fallback in
  the HIR layer.
- Reworked the main parser transport helpers in
  [parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
  so nested template-arg rebuild and dependency probing read structured
  `TemplateArgRef` payloads directly instead of routing through the generic
  `encode_template_arg_debug_list(...)` flattening helper in those common
  paths.
- Reworked the remaining key-centric callers in
  [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
  and [types_helpers.hpp](/workspaces/c4c/src/frontend/parser/types_helpers.hpp)
  so pending-type and canonical-template identity strings are now encoded from
  structured `TemplateArgRef` payloads via key-specific local logic rather
  than by reusing the generic debug-list helper.
- Reworked [hir_templates.cpp](/workspaces/c4c/src/frontend/hir/hir_templates.cpp)
  so `encode_template_type_arg_ref_hir(...)` now emits nested compatibility
  refs from structured `TemplateArgRef` payloads directly instead of routing
  them through `encode_template_arg_debug_list(...)`.
- Reworked [hir_expr.cpp](/workspaces/c4c/src/frontend/hir/hir_expr.cpp) so
  zero-arg `operator()` calls on struct objects can fold straight to a known
  static `value`, fixing the EASTL signed-helper base-expression regression
  without depending on inherited base-method lowering to materialize the exact
  `integral_constant<bool, v>` specialization first.
