Status: Active
Source Idea Path: ideas/open/174_aggregate_abi_classification_structured_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select the Aggregate ABI Boundary

# Current Packet

## Just Finished

Completed Step 1: selected the BIR call-argument aggregate ABI boundary in
`BirFunctionLowerer::lower_call_inst`.

Selected boundary:
- BIR lowering of direct and indirect aggregate byval call arguments:
  `src/backend/bir/lir_to_bir/calling.cpp`
  `BirFunctionLowerer::lower_call_inst`, plus helper parsing in
  `BirFunctionLowerer::parse_typed_call` and
  `BirFunctionLowerer::parse_direct_global_typed_call`.
- Shared layout/classification helper:
  `src/backend/bir/lir_to_bir/aggregate.cpp`
  `BirFunctionLowerer::lower_byval_aggregate_layout`.
- ABI records populated from that boundary:
  `src/backend/bir/bir.hpp` `CallArgAbiInfo` and the `CallInst::arg_abi`
  vector.

Structured facts already available there:
- `LirCallOp::arg_type_refs` mirrors call argument type fragments and can carry
  `StructNameId` for aggregate arguments.
- `LirCallOp::return_type` already carries a structured return `LirTypeRef`;
  this gives a nearby model for preserving final spelling while using
  structured metadata.
- `context_.lir_module.struct_names` and `structured_layouts_` are available in
  `lower_call_inst`, and existing aggregate layout lookup already accepts
  structured layout tables.
- The lowered BIR surface already records explicit ABI facts in
  `CallArgAbiInfo`: `size_bytes`, `align_bytes`, `primary_class`,
  `byval_copy`, and `sret_pointer`.

Spelling-derived authority to replace or guard:
- `src/backend/bir/lir_to_bir/calling.cpp` reparses call text with
  `parse_lir_typed_call_args`, `parse_lir_call_param_types`, and local
  `parse_byval_pointee_type`, then uses the parsed type spelling in
  `lower_call_inst` to decide byval aggregate layout and populate
  `CallArgAbiInfo`.
- The next implementation should prefer `call.arg_type_refs[index]` for
  metadata-rich aggregate byval arguments, using rendered call text only as
  final output/legacy no-metadata compatibility.

## Suggested Next

Implement Step 2 for the selected boundary: add a small structured aggregate
layout lookup path for `LirCallOp::arg_type_refs[index]` in
`BirFunctionLowerer::lower_call_inst`, then route direct and indirect byval
call-argument `CallArgAbiInfo` population through that structured lookup when
`arg_type_refs` are present.

Suggested focused proof command for the supervisor to delegate next:

```bash
cmake --build build --target backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|frontend_lir_call_type_ref|backend_codegen_route_x86_64_(byval_member_array_params|aggregate_param_return_pair|indirect_aggregate_param_return_pair|aggregate_param_return_pair_fn_param)_observe_semantic_bir)$' --output-on-failure > test_after.log
```

## Watchouts

- Keep the old parsed-call/byval text route only as an explicit legacy fallback
  when `LirCallOp::arg_type_refs` is empty.
- If `arg_type_refs` is present but its size mismatches parsed call args, or an
  aggregate mirror has a missing/stale `StructNameId`, do not silently fall back
  to `parse_byval_pointee_type`; fail the call family or add an adjacent
  verifier/diagnostic path in the next packet.
- Do not extend the local `parse_byval_pointee_type` parser or add a new
  rendered-signature parser as the structured classification mechanism.
- Function parameter lowering in `call_abi.cpp` already prefers structured
  signature metadata; the selected packet should avoid broad rewrites there
  except for shared helper extraction if needed.

## Proof

Inspection and `todo.md` update only. Per packet contract, no build or test
proof was required or run, and no `test_after.log` was written.
