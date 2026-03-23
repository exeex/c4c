# Plan Execution State

## Active Item
EASTL `type_traits.h` bring-up

## Completed This Session
- Added compile-time-engine-side deferred template-arg helpers for nested/deferred NTTP expression refs (`$expr:...`) and arg-ref splitting
- Parser now preserves dependent NTTP template args as deferred expr refs instead of immediately forcing them through integer fallback
- Confirmed the remaining wrong instantiation path goes through template alias application (`using bool_constant = integral_constant<bool, B>`)
- Parser pending-template base instantiation now substitutes forwarded NTTP names in `tpl_struct_arg_refs` instead of silently falling back to `0`
- HIR constructor-expression type inference now recognizes `StructName(args...)` calls as struct-valued expressions
- HIR now caches instantiated static constexpr member values with NTTP substitution so inherited `::value` lookup can reuse concrete values once the right base chain exists
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
  - next step: represent NTTP expression template args like `bool_constant<(T(-1) < T(0))>` as deferred arg refs/text instead of collapsing them early
  - current confirmed failure mode: parser now preserves the deferred expr, but alias-template application still loses it because `using bool_constant = integral_constant<bool, B>` does not preserve alias template param order/bindings in reusable metadata
  - next concrete slice: add alias-template instantiation metadata so applying `bool_constant<expr>` can rebuild `integral_constant<bool, expr>` before compile-time-engine lazy resolution
  - then resolve those deferred NTTP arg refs when bindings become concrete during lazy template struct instantiation
  - implementation detail:
    `declarations.cpp` / parser alias registration needs a first-class alias-template record, not only `typedef_types_`
  - implementation detail:
    alias-template record should carry:
    alias name, template param names/order, which params are NTTP, the aliased `TypeSpec`, and deferred arg refs for the aliased template if it targets a template struct
  - implementation detail:
    applying `bool_constant<expr>` should produce a pending `TypeSpec` whose `tpl_struct_origin` is `integral_constant`, and whose `tpl_struct_arg_refs` become `bool,$expr:(T(-1)<T(0))` instead of eagerly instantiating `integral_constant<bool,0>`
  - implementation detail:
    parser may still store `$expr:...` text, but it should stop trying to semantically evaluate those expression args during alias application
  - implementation detail:
    actual evaluation of `$expr:...` should happen only when `resolve_pending_tpl_struct(...)` is called with concrete `TypeBindings` / `NttpBindings`
  - implementation detail:
    that resolver should move behind compile-time-engine-owned helpers so expression parsing/splitting is shared and no longer ad-hoc to `ast_to_hir`
  - implementation detail:
    expression evaluator only needs a narrow initial surface for this EASTL slice:
    integer literals, bool literals, forwarded NTTP names, simple casts like `T(-1)`, arithmetic/comparison/logical operators, and `Template<Args>::value`
  - implementation detail:
    once a concrete NTTP value is obtained, specialization selection must run on the rebuilt concrete arg list before any field/method/base propagation
  - implementation detail:
    static constexpr member caching should remain a post-instantiation optimization only; it must not be the source of truth for selecting template specializations
  - implementation detail:
    add one minimal regression probe for this path first:
    `template_alias_deferred_nttp_expr_runtime.cpp`
  - implementation detail:
    after that passes, revalidate the broader probes:
    `inherited_static_member_lookup_runtime` and `eastl_type_traits_signed_helper_base_expr_parse`
- `is_signed_helper` runtime: operator() call chain through inherited template methods
  - `eastl_type_traits_signed_helper_base_expr_parse` still fails at runtime (`exit=no such file or directory`)
  - current state: constructor-like call expressions now infer as struct-valued in HIR, but inherited `operator()` still cannot dispatch because the base chain still resolves to the wrong instantiated trait
- After runtime fix: resolve inherited `::value` for template instantiations (base_tags propagation / correct base instantiation)
- Then: push to `eastl_type_traits_simple_workflow` full green

## Exposed Failing Tests
- `eastl_type_traits_signed_helper_base_expr_parse` — runtime fails (`exit=no such file or directory`)
- `inherited_static_member_lookup_runtime` — runtime fails (`exit=1`)

## Blockers
- NTTP expression template args in type/base context are still collapsed too early instead of being lazily instantiated after bindings are concrete
- Template alias application currently loses deferred NTTP expression refs because alias template parameter metadata is not preserved alongside the aliased `TypeSpec`
- Inherited static member `::value` on template instantiations depends on the correct deferred base instantiation result
- Template struct operator() call resolution in expressions still needs inherited method dispatch once the right base chain exists

## Detailed Execution Notes
- The short-term goal is not “support arbitrary constexpr expressions everywhere”.
  The goal is narrower: preserve dependent NTTP expressions long enough that template struct instantiation can evaluate them after bindings are concrete.
- The boundary should be:
  parser records syntax, compile-time-engine helpers own deferred arg interpretation, and HIR instantiation consumes concrete results.
- Expected data flow:
  1. parser sees `bool_constant<(T(-1) < T(0))>`
  2. alias-template application rebuilds it as pending `integral_constant<bool, $expr:(T(-1)<T(0))>`
  3. lazy template-struct instantiation receives concrete `T`
  4. compile-time-engine helper evaluates `$expr:(int(-1)<int(0))` to `1`
  5. specialization selection / mangling chooses `integral_constant_T_bool_v_1`
  6. derived structs inherit correct `base_tags`, `::value`, and `operator()`
- Avoid:
  mutating AST nodes in place just to borrow default-expression storage,
  silently converting unknown NTTP exprs to `0`,
  and reintroducing parser-time token injection for cases that should now be lazy.
- Good intermediate success condition:
  `template_alias_deferred_nttp_expr_runtime` turns green and HIR dump shows `integral_constant<...,1>` / `integral_constant<...,0>`-equivalent concrete instantiations instead of only `_v_0`.

## Suite Status
- 2118/2120 passed (same 2 pre-existing failures, no regressions)
