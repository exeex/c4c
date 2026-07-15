Status: Active
Source Idea Path: ideas/open/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the native carrier boundary

# Current Packet

## Just Finished

- 799 Step 1 complete: the selected row is exactly the AMD64 SysV
  aggregate `layout.needs_memory` overflow path in
  `StmtEmitter::emit_amd64_va_arg_from_overflow`.  The existing
  `LirMemoryVaPointerAuthority` is intentionally local-object-only and cannot
  authorize its `stack_ptr`: `Amd64VaListPtrs::overflow_ptr_ptr` is presently
  text-only, and `stack_ptr` is the pointer loaded from that field.
- Step 2 contract: add one distinct optional AMD64-overflow aggregate carrier
  to the selected non-volatile `LirMemcpyOp` (do not relabel the source as a
  `LirCurrentFunctionLocalObjectPointer`).  Its structural relation is:
  `(a)` a current-function-live direct local `va_list` pointer/object/owner;
  `(b)` a typed `LirGepOp` result whose base is that pointer and whose native
  indices select AMD64 SysV `__va_list_tag` field 2;
  `(c)` a `ptr` `LirLoadOp` from that exact field-address result, whose result
  is the memcpy source; and `(d)` storage identity
  `{va_list_object, Amd64SysVOverflowArgArea}`, rather than a fabricated local
  source object.  Preserve these as `LirValueId`/`LirObjectId`/`LinkNameId`/
  `LirTypeRef` fields, not rendered operands or names.
- The same carrier binds one aggregate `payload_type`, `i64` positive
  `payload_size`, the memcpy's immediate `size`, the destination temporary
  alloca's current-function-live local-object authority, and the final load's
  aggregate type.  Step 2 must only select when all three type/size mirrors
  agree: carrier payload type = temporary alloca pointee type = final load
  type, and carrier typed byte immediate = memcpy typed `i64` size immediate.
- Verifier rejects: a source not equal to the field-2 load (non-overflow
  derived); a non-pointer/malformed GEP-load chain or wrong base/storage kind;
  invalid, foreign, non-unique, or dead `va_list` owner/object or destination;
  any payload/temporary/load type disagreement; and non-`i64`, non-positive,
  or mismatched byte sizes.  `requires_native_memory_va_authority` remains
  fail-closed: no partial/new carrier fields on an unselected memcpy.

## Suggested Next

- Implement only that carrier: make the AMD64 access record retain typed
  field-address/load identities, attach the live direct-local `va_list` base
  and destination alloca authority, and extend the LIR verifier.  Add the
  accepted direct-local aggregate-overflow lowering case plus mutations for
  non-overflow-derived, foreign, dead, type-incoherent, and size-incoherent
  carriers in `tests/backend/bir/backend_lir_selected_pointer_authority_test.cpp`.

## Watchouts

- Selection gate is AMD64 SysV + aggregate + positive `layout.needs_memory` +
  a checked direct local `va_list` base.  Register-path aggregates, scalar and
  pointer `va_arg`, other targets, indirect/nonlocal `va_list`, and unsupported
  aggregate forms keep their current compatibility-only behavior.  Do not use
  `LirSelectedMemcpyAuthority`: it is the fixed-byval row with two local
  objects, not this derived overflow storage relation.

## Proof

- Step 1 analysis only: `git diff --check` passed.  Nearest focused seam for
  Step 2 is `backend_lir_selected_pointer_authority_test`; supervisor must
  select its fresh-build and focused-test command and preserve `test_after.log`.
  Preserve 753's accepted 3037/3037 full-baseline reference.
