Status: Active
Source Idea Path: ideas/open/174_aggregate_abi_classification_structured_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Route Metadata-Rich Inputs Through Structured Facts

# Current Packet

## Just Finished

Completed Step 2: routed BIR aggregate byval call-argument ABI classification
through structured facts at the selected `BirFunctionLowerer::lower_call_inst`
boundary.

Implementation notes:
- Added a local `lower_byval_call_arg_layout` path in
  `src/backend/bir/lir_to_bir/calling.cpp` that treats nonempty
  `LirCallOp::arg_type_refs` as authoritative for byval aggregate layout
  classification.
- Metadata-rich call arguments now resolve `StructNameId` through
  `context_.lir_module.struct_names`, then use the corresponding
  `structured_layouts_` entry to populate `CallArgAbiInfo` size, alignment,
  memory class, and `byval_copy`.
- The rendered parsed type spelling remains the explicit legacy compatibility
  path only when `call.arg_type_refs` is empty.
- Direct and indirect call byval aggregate branches both use the same structured
  lookup helper; scalar and public pointer call-argument paths are unchanged.

## Suggested Next

Exercise the next coherent packet against producer coverage: either extend or
inspect the LIR call-argument mirror producer for fixed byval aggregate
arguments, then prove metadata-rich byval calls actually arrive with usable
`StructNameId` mirrors instead of only the legacy empty-`arg_type_refs` route.

## Watchouts

- Nonempty `arg_type_refs` now fail the direct or indirect call family if an
  aggregate byval argument lacks a usable `StructNameId` or structured layout.
  That is intentional: missing metadata should not silently fall back to
  rendered type parsing.
- Existing generated fixed byval aggregate calls may still need producer work if
  their `OwnedLirTypedCallArg::type_ref` is empty; this packet only implemented
  the selected BIR boundary route.
- Keep any producer follow-up semantic. Do not add byval-string parsing or
  named-testcase shortcuts.

## Proof

Ran the delegated proof command exactly:

```bash
cmake --build build --target backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|frontend_lir_call_type_ref|backend_codegen_route_x86_64_(byval_member_array_params|aggregate_param_return_pair|indirect_aggregate_param_return_pair|aggregate_param_return_pair_fn_param)_observe_semantic_bir)$' --output-on-failure > test_after.log
```

Result: passed. `test_after.log` contains 6/6 passing tests.
