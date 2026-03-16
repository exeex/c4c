# Plan Execution State

## Completed
- **Phase 1**: Recursive CanonicalType model — done (canonical_symbol.hpp/cpp exist, compile, convert from TypeSpec)
- **Phase 2**: Resolved type context — done
  - `ResolvedTypeTable` with per-Node canonical type mapping
  - Populated during `build_canonical_symbols` for functions, globals, params, struct/enum defs
  - Stored in `SemaCanonicalResult` → accessible from `AnalyzeResult`
  - `lookup()` / `record()` query helpers
  - `format_canonical_type()` / `format_canonical_result()` debug formatters
  - `--dump-canonical` CLI flag for inspection
  - Fixed struct/enum def canonicalization (proper nominal types)
  - Fixed fn_ptr return type canonicalization in `canonicalize_function_node_type`
- **Phase 3**: Migrate callable/prototype logic — done
  - Added `is_callable_type()` and `get_function_sig()` canonical type helpers
  - Added `typespec_from_canonical()` reverse conversion (canonical → TypeSpec)
  - Threaded `ResolvedTypeTable` to `lower_ast_to_hir` via new parameter
  - Added `canonical_sig` field to `FnPtrSig` (shared_ptr<CanonicalType>)
  - `fn_ptr_sig_from_decl_node()` now derives FnPtrSig from canonical type when available
  - Falls back to legacy parser-based extraction when canonical type is absent
  - All 1784 tests pass with no regressions
- **Phase 4**: Migrate lowering hooks — done
  - Added canonical-aware helper functions in hir_emitter.cpp:
    - `sig_return_type()`: extract return TypeSpec preferring canonical_sig
    - `sig_param_type()`: extract i-th param TypeSpec preferring canonical_sig
    - `sig_param_is_va_list_value()`: va_list detection via canonical path
    - `sig_param_count()`: param count via canonical_sig or legacy
    - `sig_is_variadic()`: variadic check via canonical_sig or legacy
    - `sig_has_void_param_list()`: void param list check via canonical path
  - Modified return type resolution (`resolve_payload_type(CallExpr)`) to use `sig_return_type()`
  - Modified argument coercion loop to use `sig_param_type()`, `sig_is_variadic()`, `sig_has_void_param_list()`
  - Modified va_list detection in indirect calls to use `sig_param_is_va_list_value()`
  - Modified local fn-ptr return type caching to use `sig_return_type()`
  - Marked legacy TypeSpec-based prototype recovery paths with TODO(phase5) comments
  - Updated FnPtrSig doc comment in ir.hpp
  - All 1784 tests pass with no regressions

## Known Limitations (pre-existing)
- Functions returning function pointers: parser does not fully capture fn_ptr params on the function node, so canonicalization inherits that gap
- Expression-level canonical type tracking not yet implemented (only declaration nodes)
- Local variable declarations not yet in ResolvedTypeTable (canonical path falls back to legacy for locals)
- Legacy TypeSpec-based return type recovery paths retained for undeclared extern calls and expression-level fn-ptrs

## Next Intended Slice
- **Phase 5**: Connect canonical symbol building
  - Build canonical declaration identity from sema-resolved canonical types
  - Ensure function prototypes and function definitions map to the same canonical function identity
  - Prepare template declaration vs instantiation distinction
  - Forbid parser-only type fragments from acting as mangling input

## Deferred
- Phase 6: Enable mangling
