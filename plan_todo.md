# Plan Execution State

## Baseline
- Test suite: **1800/1800** (2026-03-16, after Phase D slice 2)

## Completed
- Phase A: Lock Current Behavior With Tests
- Phase B: Fill `ResolvedTypeTable` For Locals
- Phase C slice 1: Typedef fn_ptr param propagation + struct member fn_ptr sig
- Phase C slice 2: CastExpr, IndexExpr, CallExpr fn_ptr sig resolution
- Phase C slice 3: ret_fn_ptr_sig zero-param fix + CallExpr cleanup
- Cleanup item 1: Parser gap for nested fn-returning-fn-ptr declarators
  - Added `ret_fn_ptr_params` fields to Node for nested fn_ptr return info
  - Parser captures both levels of fn_ptr params in nested declarators
  - XFAIL test flipped to pass
- Phase D slice 1: Remove `local_fn_ret_types` map + legacy TypeSpec peeling
  - Removed `local_fn_ret_types` map from FnCtx (declaration, population, all usage)
  - Removed legacy TypeSpec peeling fallback in CallExpr return type resolution
  - Simplified `sig_return_type` to use `sig.return_type.spec` directly
  - Fixed canonical path in `fn_ptr_sig_from_decl_node` with ret_fn_ptr_params correction
  - Reordered CallExpr resolution: fn_index → canonical fn_ptr_sig → known libc → implicit int
- Phase D slice 2: Move DeclRef fn_index type resolution to HIR lowering
  - Pre-register functions in fn_index before body lowering (self-reference support)
  - NK_VAR handler annotates DeclRef with fn_ptr type from fn_index
  - Removed codegen fallback in `resolve_payload_type(DeclRef)` that built synthetic TypeSpec
  - Tightened local fn prototype skip (require n_params > 0) to prevent variable/function name collision

## Active Item
- **Phase D: Delete Legacy Callable Recovery** (remaining slices)

## Next Slice
- Audit remaining `resolve_payload_type` overloads for legacy TypeSpec reconstruction
- Consider removing `sig_*` helper canonical fallbacks (sig_param_type, sig_param_count, etc.) — canonical_sig is still needed for param extraction
- Remove remaining legacy comment references
- Evaluate if all Phase D exit conditions are met

## Blockers
- None known

## Deferred
- (none)
