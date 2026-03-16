# Plan Execution State

## Baseline
- Test suite: **1800/1800** (2026-03-16, after Phase D slice 3)

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
- Phase D slice 3: Canonicalize ret_fn_ptr_sig in lower_function
  - ret_fn_ptr_sig now built from canonical type (via canonicalize_declarator_type)
  - canonical_sig populated on ret_fn_ptr_sig (was missing before)
  - Nested fn_ptr fixup applied same as fn_ptr_sig_from_decl_node
  - Updated Phase 4 comments → final canonical approach language

## Active Item
- **Phase D: Delete Legacy Callable Recovery** (evaluation)

## Next Slice
- Evaluate Phase D exit conditions:
  - All FnPtrSig creation paths now populate canonical_sig (fn_ptr_sig_from_decl_node canonical path, struct fields, ret_fn_ptr_sig)
  - Only remaining legacy path: fn_ptr_sig_from_decl_node fallback (for nodes not in ResolvedTypeTable)
  - sig_* helper fallbacks still present as safety net but canonical_sig should always be set
- Consider: mark Phase D complete if exit conditions substantially met
- Remaining gap: legacy path in fn_ptr_sig_from_decl_node (needed for safety when canonical lookup fails)

## Blockers
- None known

## Deferred
- (none)
