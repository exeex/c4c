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

## Known Limitations (pre-existing)
- Functions returning function pointers: parser does not fully capture fn_ptr params on the function node, so canonicalization inherits that gap
- Expression-level canonical type tracking not yet implemented (only declaration nodes)
- Local variable declarations not yet in ResolvedTypeTable (canonical path falls back to legacy for locals)

## Next Intended Slice
- **Phase 4**: Migrate lowering hooks
  - Replace lowering-side function-signature reconstruction with canonical type access
  - Use canonical type for indirect call lowering in `hir_emitter.cpp`
  - Reduce dependence on duplicated `FnPtrSig`-style side structures
  - Delete redundant lowering-side TypeSpec prototype recovery once parity is established

## Deferred
- Phase 5: Connect canonical symbol building
- Phase 6: Enable mangling
