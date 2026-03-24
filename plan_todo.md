# Plan Todo — Lazy Template Type Instantiation

## Status: In Progress (2026-03-24)

## Plan Item
Lazy template type instantiation (template_lazy_instantiation_plan.md Steps 1-2)

## Completed
- Fix: deferred NTTP default with binary expressions (is_void<T>::value || ...)
  - Parser's `eval_deferred_nttp_default` refactored to token-based recursive descent
    supporting ||, &&, ==, !=, <, >, <=, >=, !, - operators over member lookups
  - Parser defers template struct instantiation when NTTP defaults can't be evaluated
    (`has_incomplete_nttp_default` condition)
  - Expression parser: when template struct is deferred, use `tpl_struct_origin`
    (original template name) instead of mangled tag for scope-qualified member access
  - HIR `resolve_pending_tpl_struct` correctly fills in NTTP defaults with concrete bindings
  - Fixed: `cpp_positive_sema_template_nttp_default_runtime_cpp`

## Baseline
2122/2123 tests passing (1 pre-existing failure: eastl_type_traits_signed_helper_base_expr_parse)

## Next
- Investigate EASTL type_traits signed helper test (operator() through template inheritance)
- Continue lazy instantiation plan Step 1: broader use-site enqueue coverage
- Or start Step 2: move engine control ownership
