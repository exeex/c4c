Status: Active
Source Idea Path: ideas/open/184_direct_call_signature_metadata_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory direct-call signature lowering

# Current Packet

## Just Finished

Step 1 source audit completed: inventoried generated direct-call LIR
construction, LIR-to-BIR direct-call lowering, structured facts already
available, and rendered-text fallback points.

Direct-call construction route:

- `StmtEmitter::resolve_call_target_info` in
  `src/codegen/lir/hir_to_lir/call/target.cpp` classifies direct calls through
  `CallTargetInfo::target_fn`, records `callee_link_name_id`, resolves
  `callee_val` to the emitted global symbol, and builds `callee_type_suffix`
  with `llvm_fn_type_suffix_str`.
- `StmtEmitter::emit_void_call` and `StmtEmitter::emit_call_with_result` create
  `LirCallOp` through `make_lir_call_op_with_return_type_ref`.
- `make_lir_call_op_with_return_type_ref` in `src/codegen/lir/call_args_ops.hpp`
  stores `return_type`, `callee`, `direct_callee_link_name_id`,
  rendered `callee_type_suffix`, rendered `args_str`, structured
  `arg_type_refs`, and optional `callee_signature`.
- `prepare_call_arg`/`prepare_call_args` in
  `src/codegen/lir/hir_to_lir/call/args.cpp` provide typed argument spelling
  and `LirTypeRef` mirrors for aggregate-sensitive call arguments.

Direct-call lowering route:

- `BirFunctionLowerer::lower_call_inst` in
  `src/backend/bir/lir_to_bir/calling.cpp` detects a direct global callee with
  `parse_lir_direct_global_callee`, resolves semantic identity through
  `direct_callee_link_name_id`, and uses `parse_direct_global_typed_call` for
  the direct-call argument/signature view.
- `BirFunctionLowerer::parse_direct_global_typed_call` currently derives
  fixed/variadic parameter text from `call.callee_type_suffix`, then parses
  rendered `args_str`; byval pointee recovery uses `parse_byval_pointee_type`.
- `lower_call_inst` then lowers scalar/pointer args, byval aggregate args,
  sret returns, direct memory intrinsics, and the final `bir::CallInst`.

Structured fact sources already present:

- `LirCallOp::direct_callee_link_name_id` is the structured direct callee
  identity and is used by BIR lowering when present.
- `LirCallOp::return_type` is a `LirTypeRef`; aggregate return identity can
  carry a struct/union name id and is copied to
  `bir::CallInst::structured_return_type_name`.
- `LirCallOp::arg_type_refs` mirrors generated call-argument type fragments;
  `lower_byval_call_arg_layout` already treats non-empty mirrors as the
  structured route for aggregate byval layout and treats an empty vector as
  legacy hand-built LIR compatibility.
- `LirFunction::signature_return_type_ref`, `signature_params`,
  `signature_param_type_refs`, `signature_is_variadic`, and
  `signature_has_void_param_list` are populated by
  `populate_signature_type_refs` for function definitions/declarations and
  consumed by `lower_signature_return_info`,
  `has_structured_signature_params`, `structured_signature_params`, and
  `lower_function_params_with_layouts`.
- `LirCallSignature` plus `structured_callee_signature` exists for indirect
  function-pointer calls; direct calls currently do not populate this optional
  structured callee signature.

Rendered-signature fallback points:

- `parse_lir_direct_global_typed_call(call)` and
  `BirFunctionLowerer::parse_direct_global_typed_call` parse
  `callee_type_suffix` and `args_str` for direct-call parameter/variadic/byval
  authority.
- `parse_typed_call(call)` falls back to
  `parse_lir_call_param_types(call.callee_type_suffix)` and
  `parse_lir_typed_call_or_infer_params(call.callee_type_suffix,
  call.args_str)` unless `call.callee_signature` is present.
- `lower_byval_call_arg_layout` intentionally parses rendered byval text only
  when `call.arg_type_refs.empty()`, documented as legacy hand-built LIR
  compatibility.
- `parse_function_signature_params(function.signature_text)` and
  `lower_signature_return_info`'s `signature_text` return-type fallback are
  compatibility routes for function declarations/definitions without structured
  signature fields, not the intended generated-path authority.
- `lower_extern_decl` still tries `decl.return_type_str` before the structured
  `decl.return_type` mirror.

Route split recorded for Step 2:

- Generated metadata-rich route: direct calls emitted from HIR through
  `resolve_call_target_info`/`emit_*call` with a non-invalid
  `direct_callee_link_name_id` where available, non-empty structured
  `return_type`, and aggregate-sensitive `arg_type_refs`. This route should
  gain or reuse structured callee/function signature metadata for return,
  fixed params, variadic state, byval, sret, and aggregate layout facts, and
  should fail closed instead of silently parsing rendered signature text when
  that metadata is stale or missing.
- Raw/no-metadata compatibility route: hand-built or imported LIR lacking
  structured mirrors, especially calls with empty `arg_type_refs` and no
  structured callee signature, may continue to use rendered
  `callee_type_suffix`, `args_str`, and `signature_text` parsing behind an
  explicit compatibility branch.

## Suggested Next

Execute Step 2 from `plan.md`: fence generated metadata-rich direct calls so
they use structured signature metadata and keep rendered signature parsing only
on the explicit raw/no-metadata compatibility route.

## Watchouts

- Do not add a new rendered signature parser branch.
- Do not let generated metadata-rich calls silently fall back to string
  parsing.
- Preserve explicit raw/no-metadata compatibility rather than deleting it as a
  side effect of the generated-path fence.
- Direct calls have structured callee identity and aggregate mirrors today, but
  `callee_signature` is currently only populated for indirect function-pointer
  calls; Step 2 likely needs a direct-call structured signature source rather
  than extending `callee_type_suffix` parsing.
- `lower_extern_decl` still prefers `return_type_str` before `return_type`; this
  is adjacent compatibility behavior, not necessarily the direct-call Step 2
  repair point.
- Backend freeze closure remains owned by idea 188.

## Proof

Source-level audit only, per packet. No build/tests were run and no
`test_after.log` was produced. Local validation:
`git diff --check -- todo.md` passed.
