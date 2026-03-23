# Plan Execution State

## Active Item
EASTL `type_traits.h` bring-up

## Completed This Session
- Parser: struct-scope `using Name = Type;` now parses and stores the real alias `TypeSpec` instead of placeholder `int`
- Parser/HIR: builtin template arg refs in deferred/template-member paths now preserve mangled builtin names like `uint`/`void`
- Parser: started preserving unresolved `TemplateLike<...>::type` as deferred member-type info instead of silently dropping the `::type` suffix
- Added `AliasTemplateInfo` struct to `parser.hpp` with param names, NTTP flags, and aliased TypeSpec
- Registered alias template metadata in `declarations.cpp` after `template<...> using Name = ...`
- Added alias template application in `parse_base_type()` (types.cpp):
  - Parses alias template args (type args and NTTP args including $expr: capture)
  - Substitutes alias params in aliased type's `tpl_struct_arg_refs`
  - Rebuilds tag (mangled name) from substituted args
  - Returns pending TypeSpec with `tpl_struct_origin` pointing to aliased template struct
- Added variadic alias template fallback: when more args than alias params, restores parser position and falls through to regular template struct instantiation
- Fixed builtin type arg_refs: use `append_type_mangled_suffix` when `ats.tag == nullptr` (e.g., `bool`)
- Fixed `$expr:` text substitution: whole-word replace template param names with concrete type names
- Fixed `is_constexpr` not propagated to field nodes during template struct instantiation
- Fixed `static constexpr` scanning loop: changed `break` to `consume(); continue;` for both keywords
- Updated arg_refs on instantiation even when `all_resolved = false`
- HIR: concrete builtin type resolution in `resolve_pending_tpl_struct` (e.g., "bool" → TB_BOOL)
- HIR: pending base type resolution in `lower_struct_def` using struct's own template bindings
- HIR: ExprParser builtin cast fallback via `parse_builtin_typespec_text`
- Sema: optimistic acceptance for `StructTag::member` when base chain has unresolved $expr: parts
- Added test `template_alias_deferred_nttp_expr_runtime.cpp` (test 321) — passes

## Previous Session Completed
- Added compile-time-engine-side deferred template-arg helpers for nested/deferred NTTP expression refs
- Parser now preserves dependent NTTP template args as deferred expr refs
- Paren-aware angle bracket skipping in using alias template args
- Template struct field cloning: `is_static`, `is_constexpr`, and `init` now propagated
- Deferred NTTP default expression evaluation via token sequences
- Base class pending template resolution with deferred NTTP defaults and specialization selection

## Next Work
- Finish deferred member-type/base-chain realization for `is_signed_helper<T>::type` so concrete helper/base structs materialize before inherited `::value` / `operator()` lookup
- Fix test 212 (`eastl_type_traits_signed_helper_base_expr_parse`): runtime failure likely needs further base chain resolution or $expr: evaluation improvements
- Fix test 218 (`inherited_static_member_lookup_runtime`): needs inherited `::value` chain through correct base instantiation
- `is_signed_helper` runtime: operator() call chain through inherited template methods
- Then: push to `eastl_type_traits_simple_workflow` full green

## Exposed Failing Tests
- `eastl_type_traits_signed_helper_base_expr_parse` (212) — still fails; HIR now preserves `is_signed_T_uint` / `is_signed_T_void`, but emitted IR still lowers inherited `operator()` calls incorrectly
- `inherited_static_member_lookup_runtime` (218) — still fails; inherited `::value` path still missing concrete helper/base materialization for non-`int` cases
- `operator_shift_call_arg_runtime` (292) — runtime fails

## Suite Status
- 2119/2122 passed (3 pre-existing failures, no regressions)
