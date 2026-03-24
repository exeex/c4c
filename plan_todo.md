# Plan Todo — Template Instantiation Follow-up

## Status: In Progress (2026-03-24)

## Plan Item
Follow up on template instantiation work after the deferred NTTP default fix:
- keep template-function identity docs aligned with the completed cutover
- keep lazy template type instantiation moving forward from the new baseline

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
- Template-function identity scaffolding is already landed
  - structured function-template identity types exist in `compile_time_engine.hpp`
  - owner-based function specialization registration exists
  - lowering and deferred instantiation already use structured specialization selection
  - legacy specialization lookup path has been removed from active control flow
  - semantic dedup now keys on `primary_def + spec_key`
  - targeted template-function identity regressions passed after the cutover

## Baseline
2122/2123 tests passing (1 pre-existing failure: eastl_type_traits_signed_helper_base_expr_parse)

## Next
- Keep planning docs aligned with the cutover that is now complete
- Investigate EASTL type_traits signed helper test
  (`eastl_type_traits_signed_helper_base_expr_parse`, operator() through template inheritance)
- Continue lazy template instantiation from the new baseline
  - Step 1: broader use-site enqueue coverage
  - Step 2: move engine control ownership
