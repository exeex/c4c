# Plan Execution State

## Active Item
EASTL `type_traits.h` bring-up

## Completed This Session
- Added runtime regression test `template_nttp_default_runtime` to verify NTTP defaults select the correct specialization, not just parse successfully
- Fixed deferred NTTP default runtime resolution for pending template structs and `Template<Args>::static_member` expression lookup
- `template_nttp_default_runtime` now passes: omitted NTTP args resolve to the correct default-selected specialization at runtime
- Compile-time state now registers template struct primaries and specializations as groundwork for moving deferred NTTP expression resolution out of ad-hoc HIR-only logic
- Deferred NTTP default expression evaluation: complex defaults like `bool = arithmetic<T>::value` are now stored as token sequences and evaluated during template instantiation
- Paren-aware angle bracket skipping in using alias template args: `<` inside `()` no longer counted as template brackets (fixes `bool_constant<(T(-1) < T(0))>`)
- Template struct field cloning: `is_static`, `is_constexpr`, and `init` now propagated during instantiation
- Deferred NTTP default tokens stored on parser, keyed by template name + param index
- Token injection approach for triggering template instantiation during deferred default evaluation
- Base class pending template resolution now fills deferred NTTP defaults and selects specializations properly

## Previous Session Completed
- Fixed `static constexpr` struct members: parser now sets `is_static` flag and parses init expr
- Fixed `Template<Args>::member` qualified access in expressions to use mangled struct tag
- Fixed `Template<Args>::type` base class resolution: `parse_base_type()` now resolves `::member` on template instantiation using template origin typedef
- Fixed dependent base class recovery: unconsumed tokens after base class expression are now skipped to find struct body `{}`
- EASTL type_traits.h errors reduced from 5 to 0 frontend parse errors

## Next Work
- Move deferred NTTP expression/default resolution toward lazy instantiation / compile-time state
  - current HIR-side evaluator is enough for `template_nttp_default_runtime`, but `212/218` show it is the wrong long-term layer
  - next step: represent NTTP expression template args like `bool_constant<(T(-1) < T(0))>` as deferred arg refs instead of collapsing them early
  - then resolve them when bindings become concrete during lazy template struct instantiation
- `is_signed_helper` runtime: operator() call chain through inherited template methods
  - `eastl_type_traits_signed_helper_base_expr_parse` currently fails at runtime (`exit=no such file or directory`)
  - current state: temporary struct materialization works, but inherited `operator()` still does not dispatch correctly
- After runtime fix: resolve inherited `::value` for template instantiations (base_tags propagation)
- Then: push to `eastl_type_traits_simple_workflow` full green

## Exposed Failing Tests
- `eastl_type_traits_signed_helper_base_expr_parse` — runtime fails (`exit=no such file or directory`)
- `inherited_static_member_lookup_runtime` — runtime fails (`exit=1`)

## Blockers
- NTTP expression template args in type/base context are still collapsed too early instead of being lazily instantiated after bindings are concrete
- Inherited static member `::value` on template instantiations depends on the correct deferred base instantiation result
- Template struct operator() call resolution in expressions still needs inherited method dispatch once the right base chain exists

## Suite Status
- 2118/2120 passed (same 2 pre-existing failures, no regressions)
