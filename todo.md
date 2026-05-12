# Current Packet

Status: Active
Source Idea Path: ideas/open/180_aarch64_direct_lir_aggregate_type_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory AArch64 Direct-LIR Aggregate Text Bridges

## Just Finished

Step 1 inventory completed for AArch64 direct-LIR aggregate type text bridges.

Inspected sites and classification:
- `src/backend/mir/aarch64/codegen/emit.cpp`: AArch64 direct-LIR route gate. `lir_module_needs_nonminimal_lowering(...)` detects nonminimal type use, and `emit_prepared_lir_module(...)` validates the LIR module before falling back through shared `emit_target_lir_module(...)`/LIR-to-BIR when direct native slices cannot handle the module. Classification: route/display/output use; not the aggregate semantic authority point.
- `src/backend/backend.cpp`: non-x86 `emit_target_lir_module(...)` first calls `try_lower_to_bir_with_options(...)`, then renders prepared BIR text when lowering succeeds, otherwise keeps bootstrap LIR printing. Classification: AArch64 direct-LIR bridge route; aggregate metadata enforcement should happen before this can silently rely on rendered aggregate text.
- `src/backend/bir/lir_to_bir/module.cpp`: builds `type_decls` from legacy text, builds `BackendStructuredLayoutTable` from `LirStructDecl` plus `StructNameTable`, reports parity notes, then lowers globals, extern decls, declarations, and function bodies with the same table. Classification: metadata-rich generated input setup.
- `src/backend/bir/lir_to_bir/types.cpp`: `compute_aggregate_type_layout(...)` is the current rendered-text parser; `lookup_backend_aggregate_type_layout_result(...)` prefers structured layout by `%struct...` spelling but falls back to legacy text; `lookup_backend_aggregate_type_ref_layout_result(...)` uses `LirTypeRef::StructNameId` and fails on stale rendered text mismatch. Classification: current rendered-text authority point plus structured authority point.
- `src/backend/bir/lir_to_bir/aggregate.cpp`: `lower_byval_aggregate_layout(...)` normalizes byval/aggregate text and currently accepts `selected_aggregate_type_layout(...)` results from rendered text unless callers provide stronger structured constraints. Classification: common aggregate text bridge.
- `src/backend/bir/lir_to_bir/calling.cpp`: call lowering has an explicit no-id fixture boundary: when `call.arg_type_refs.empty()`, rendered byval text remains the only authority; when mirrors exist, `lower_byval_call_arg_layout(...)` requires a `StructNameId`, checks stale legacy spelling against it, and looks up the structured layout. Classification: explicit no-id fixture compatibility plus already-bounded metadata-rich call-argument guard.
- `src/backend/bir/lir_to_bir/call_abi.cpp`: function return and parameter ABI lowering still parse rendered signature/parameter type text through `lower_byval_aggregate_layout(...)`; the structured-param path is gated by signature metadata availability but still feeds parsed signature text into aggregate layout lowering. Classification: metadata-rich generated input bridge selected for enforcement.
- `src/backend/bir/lir_to_bir/globals.cpp` and `global_initializers.cpp`: globals prefer `global.llvm_type_ref` for layout and initializer lowering when present; otherwise they retain text layout parsing. Classification: mixed metadata-rich global route with existing structured type-ref entry, plus no-id/raw compatibility.
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`: local GEP lookup rejects structured text mismatch and rejects known structured names that fail structured lookup. Classification: nearby same-feature guard worth preserving.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp` and `memory/addressing.cpp`: aggregate local slots/addressing retain `type_text` for recursive layout and compatibility lookups through the structured layout table. Classification: broader local-memory aggregate bridge, not selected for the first bounded packet.
- `src/codegen/lir/verify.cpp`: verifies signature `LirTypeRef` mirrors, direct aggregate signature `StructNameId`s, byval aggregate fragments, and structured layout observations. Classification: metadata-rich input guard and diagnostic authority.
- `src/codegen/lir/lir_printer.cpp` and BIR structured type spelling context: rendered type text is still output/diagnostic spelling. Classification: display/output use, not selected for semantic enforcement.

Selected bounded route for Step 2: AArch64 direct-LIR function signature aggregate ABI lowering through shared LIR-to-BIR, specifically `lower_return_info_from_type(...)` and `lower_function_params(...)` in `src/backend/bir/lir_to_bir/call_abi.cpp`. For metadata-rich generated functions/declarations with `signature_return_type_ref` or `signature_param_type_refs`, aggregate return/byval parameter layout should be resolved by structured `LirTypeRef`/`StructNameId` and `BackendStructuredLayoutTable`; rendered parsed signature text should remain only the explicit no-id compatibility fallback.

Current rendered-text authority point: `compute_aggregate_type_layout(...)` in `src/backend/bir/lir_to_bir/types.cpp`, reached through `lower_byval_aggregate_layout(...)` and `lookup_backend_aggregate_type_layout_result(...)`.

Structured facts available on the selected route: `LirFunction::signature_return_type_ref`, `LirFunction::signature_param_type_refs`, `LirFunction::signature_params`, `LirStructDecl::name_id`, `StructNameTable`, `BackendStructuredLayoutTable`, parity mismatch notes, and verifier checks for stale/missing struct mirrors. Missing or not yet enforced: `lower_return_info_from_type(...)` does not receive the return `LirTypeRef`; parts of `lower_function_params(...)` still use parsed rendered parameter text even when structured signature metadata exists; explicit stale/missing structured metadata failures are not centralized at this ABI boundary.

Nearby same-feature guards: `calling.cpp` already fails closed for byval call args with present but invalid/mismatched `arg_type_refs`; `local_gep.cpp` rejects structured text mismatch and known structured names that fail structured lookup; `verify.cpp` rejects stale or missing signature mirrors for known aggregate returns/params; global lowering already prefers `global.llvm_type_ref` when present.

## Suggested Next

Delegate Step 2 to implement structured aggregate metadata enforcement for the
selected AArch64/direct-LIR function signature ABI route in `call_abi.cpp`.
Thread or consult signature `LirTypeRef` mirrors for aggregate returns and
parameters, preserve the explicit no-id rendered-text compatibility fallback,
and make stale/missing structured facts fail closed for metadata-rich inputs.

## Watchouts

- Do not add new rendered type parser branches.
- Do not let metadata-rich aggregate inputs silently fall back to rendered text.
- Keep hand-authored no-id direct-LIR fixture compatibility explicit.
- Avoid broad non-AArch64 ABI rewrites.
- Do not weaken the existing `calling.cpp` byval call-argument guard; it is the
  model for the selected signature ABI boundary.
- Keep rendered type text available for printer output, diagnostics, parity
  notes, and legacy/no-id fixture compatibility only.
- The selected route is shared LIR-to-BIR but enters from AArch64 direct-LIR
  through `emit_target_lir_module(...)`; keep the implementation bounded to the
  signature ABI bridge instead of rewriting all aggregate layout lookup users.

## Proof

Lifecycle/mapping-only packet; no build or CTest run required, and no
`test_after.log` was created.

Targeted proof family for the later implementation packet:
- Build: `cmake --build build --target backend_lir_to_bir_notes_test backend_aarch64_signature_metadata_test frontend_lir_function_signature_type_ref_test`
- Existing CTest subset: `ctest --test-dir build -R '^(backend_lir_to_bir_notes|backend_aarch64_signature_metadata|frontend_lir_function_signature_type_ref)$' --output-on-failure`
- Add focused coverage in the implementation/test packet for metadata-rich
  AArch64/direct-LIR aggregate return/byval parameter success and stale/missing
  structured metadata failure at the selected signature ABI boundary.
