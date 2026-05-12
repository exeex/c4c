Status: Active
Source Idea Path: ideas/open/179_byval_copy_layout_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and Select Byval Copy Route

# Current Packet

## Just Finished

Completed Step 1 inventory for the bounded byval copy/materialization route.
Selected route: incoming byval parameter materialization into BIR local aggregate
slots, from signature lowering metadata through `aggregate_params_` and
`materialize_aggregate_param_aliases`.

Concrete route:

- `src/backend/bir/lir_to_bir/call_abi.cpp`: `lower_function_params_with_layouts`
  recognizes aggregate parameters, calls `lower_byval_aggregate_layout`, and
  publishes `bir::Param{.type = Ptr, .size_bytes, .align_bytes, .abi.byval_copy
  = true, .is_byval = true}`.
- `src/backend/bir/lir_to_bir/aggregate.cpp`: `collect_aggregate_params`
  builds `AggregateParamMap` from `function_.signature_params` /
  `function_.signature_param_type_refs` when structured signature params are
  available, otherwise from `parse_function_signature_params`; it records
  `AggregateParamInfo{.type_text, .layout}`.
- `src/backend/bir/lir_to_bir/aggregate.cpp`:
  `materialize_aggregate_param_aliases` declares `%lv.param.<name>` slots,
  aliases the incoming byval pointer param to that aggregate slot base, then
  emits per-leaf `LoadLocalInst` from `param_name + byte_offset` and
  `StoreLocalInst` into local slots using `info.layout.align_bytes`.
- Adjacent local-store route in
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp::lower_memory_store_inst`
  has a similar incoming-param copy loop for `aggregate_params_`, but is not
  the first implementation route.

Available structured metadata entry point:

- `src/backend/bir/lir_to_bir/types.cpp::build_backend_structured_layout_table`
  builds `BackendStructuredLayoutTable` entries keyed by structured final type
  name with `name_id`, `structured_layout`, legacy parity flags, and mismatch
  state.
- `src/backend/bir/lir_to_bir/types.cpp::lookup_backend_aggregate_type_ref_layout_result`
  can resolve a `lir::LirTypeRef` by `StructNameId` and fail closed on stale
  text or parity mismatch.
- The selected route currently has `function_.signature_param_type_refs`
  available in `collect_aggregate_params`; these refs are the natural
  structured owner/layout input for byval parameter copy layout identity.

Current rendered-spelling/text-layout authority point:

- `AggregateParamInfo::type_text` in
  `src/backend/bir/lir_to_bir/lowering.hpp` is explicitly documented as
  compatibility LIR type text retained for byval aggregate layout.
- `collect_aggregate_params` currently passes normalized rendered type text to
  `lower_byval_aggregate_layout`, and
  `lower_byval_aggregate_layout` normalizes text then calls
  `lookup_backend_aggregate_type_layout_result(text, type_decls,
  structured_layouts)`.
- That lookup uses structured layout only when the trimmed text name hits the
  structured table key, otherwise it falls back to legacy `TypeDeclMap`
  layout. This leaves metadata-rich same-spelled/stale-spelled parameter copy
  identity tied to rendered text instead of the parameter `LirTypeRef`
  `StructNameId`.

Nearby same-feature cases to guard against overfit:

- Existing byval call-argument metadata fail-closed coverage in
  `tests/backend/backend_lir_to_bir_notes_test.cpp` around
  `expect_legacy_byval_call_arg_without_type_refs_still_lowers`,
  `expect_metadata_rich_byval_call_arg_without_struct_id_fails_closed`, and
  `expect_metadata_rich_byval_call_arg_mismatch_fails_closed`.
- Existing structured layout lookup stale/mismatch coverage in
  `tests/backend/backend_prepare_structured_context_test.cpp` for
  `lookup_backend_aggregate_type_ref_layout_result`.
- Semantic BIR route tests that observe incoming byval param copies:
  `backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir`,
  `backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir`,
  `backend_codegen_route_x86_64_indirect_aggregate_param_return_pair_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_aggregate_param_return_pair_fn_param_observe_semantic_bir`.

## Suggested Next

Step 2 implementation packet: thread `LirTypeRef` structured layout identity
through the selected incoming byval parameter materialization route only. Add or
reuse a helper that resolves `function_.signature_param_type_refs[index]` via
`lookup_backend_aggregate_type_ref_layout_result` for metadata-rich params,
keeps the current text fallback only for legacy no-metadata params, and has
`collect_aggregate_params` / `materialize_aggregate_param_aliases` consume that
resolved layout for size, alignment, and leaf slot selection.

## Watchouts

- Do not weaken byval/backend expectations.
- Do not add rendered-type parser fallbacks for metadata-rich generated inputs.
- Keep ABI classification changes out of this idea unless they are strictly required to feed the selected copy route.
- Keep the first implementation route scoped to incoming byval parameter
  materialization. Direct/indirect byval call-argument copies already have
  `call.arg_type_refs` fail-closed behavior and should remain guard coverage,
  not be reworked in the same packet.
- Preserve legacy hand-built LIR behavior when no structured signature param
  metadata is present.

## Proof

Lifecycle/mapping-only Step 1 packet; no build or CTest required and no
`test_after.log` created.

Targeted proof command family for the later implementation packet:

- Build/compile proof first: `cmake --build build --target
  backend_prepare_structured_context_test backend_lir_to_bir_notes_test`
  plus any edited backend test target.
- Narrow CTest proof: `ctest --test-dir build -R
  'backend_prepare_structured_context|backend_lir_to_bir_notes|backend_codegen_route_x86_64_(byval_member_array_params|aggregate_param_return_pair|indirect_aggregate_param_return_pair|aggregate_param_return_pair_fn_param)_observe_semantic_bir'
  --output-on-failure`.
- If x86 prepared handoff behavior changes, add `ctest --test-dir build -R
  'backend_prepare_liveness|backend_runtime_byval_helper_payload_(8_to_13|9_to_14|mixed_hfa_payload)'
  --output-on-failure`.
