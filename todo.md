# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Rebaseline remaining call-authority gaps

## Just Finished

Step 1 audit classified the remaining `calls.cpp` target helpers:

- `lower_before_call_immediate_binding`: duplicate semantic recovery needing
  replacement. It scans `call_plan.arguments` by ABI index/immediate encoding
  from a prepared binding instead of consuming a keyed prepared argument or move
  lookup.
- `find_prepared_frame_slot_call_argument_move`: duplicate semantic recovery
  needing replacement. It rescans the before-call move bundle and reclassifies
  each move locally to rediscover the argument move.
- `materialize_scalar_call_argument_value`: duplicate semantic recovery needing
  replacement. It recurses through same-block scalar producers and locally
  lowers eligible binary producers instead of consuming prepared producer/source
  authority.
- `lower_scalar_call_argument_producers`: duplicate semantic recovery needing
  replacement. It drives the same-block scalar fallback and the direct-global
  select-chain call-argument route from raw call operands plus an arg-index
  plan lookup.
- `materialize_direct_global_select_chain_call_argument`: out-of-scope adjacent
  owner for this `calls.cpp` packet. The implementation lives in
  `dispatch_edge_copies.cpp`; `calls.cpp` is currently only its call site.
- `materialize_call_boundary_source_to_destination`: duplicate semantic
  recovery needing replacement. It maps prepared memory result names back to
  BIR values through a local scan before publishing to the destination register.
- `find_bir_value_for_prepared_name`: duplicate semantic recovery needing
  replacement. It walks prior BIR instruction results and current call
  arguments by prepared name spelling.
- `materialize_indirect_call_callee_to_prepared_register`: duplicate semantic
  recovery needing replacement. It consumes the prepared indirect-callee target
  register, but still recovers source values through local-load/store and CSEL
  producer fallbacks.
- `resolve_indirect_callee_local_load_store`: duplicate semantic recovery
  needing replacement. It finds same-block named producers and prior local
  stores to rewrite the callee source.
- `emit_indirect_callee_value_to_register_with_csel`: target-local emission
  mechanic plus duplicate semantic recovery. Scratch selection and final
  `cmp`/`csel` spelling are emission mechanics; recursive select-chain source
  recovery and raw producer lookup need prepared authority.
- `record_call_result_source_register`: mixed status. The GPR result path is
  already a prepared result-plan consumer; the FPR path still scans the
  after-call move bundle and value homes, so it needs a prepared result or
  after-call result lookup replacement.

Direct variadic by-value aggregate lane publication is not the active blocker:
current call planning carries `source_selection.byval_lane_extent_bytes` for
direct variadic single-GPR lanes, and `calls.cpp` consumes the prepared selected
lane source in the aggregate publication paths.

## Suggested Next

First bounded implementation packet: execute Step 2 for
`lower_before_call_immediate_binding` and
`find_prepared_frame_slot_call_argument_move`. Replace their local argument and
move-bundle scans with existing or new prepared lookups keyed by call
instruction plus ABI argument/binding, while preserving current staging and
instruction spelling.

Suggested focused validation subset for the supervisor to delegate:
`cmake --build --preset default && ctest --test-dir build -R
"^(backend_aarch64_return_lowering|backend_prealloc_call_boundary_classification|backend_prepare_frame_stack_call_contract)$"
--output-on-failure`.

## Watchouts

Keep the first implementation packet on Step 2 only. The scalar producer,
boundary source, indirect callee, and result publication clusters are separate
packets; do not fold them into the immediate/frame-slot lookup repair. Treat
`materialize_direct_global_select_chain_call_argument` as a dispatch edge-copy
owner unless the supervisor opens an adjacent-owner packet.

## Proof

Audit-only Step 1; no build or test proof required and no `test_after.log` was
created. Read-only commands run:
`git status --short`;
`command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb`;
`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json`;
`c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json`;
`c4c-clang-tool-ccdb function-callees ... lower_before_call_immediate_binding ...`;
`c4c-clang-tool-ccdb function-callees ... materialize_scalar_call_argument_value ...`;
`c4c-clang-tool-ccdb function-callees ... lower_scalar_call_argument_producers ...`;
`c4c-clang-tool-ccdb function-callees ... find_prepared_frame_slot_call_argument_move ...`;
`c4c-clang-tool-ccdb function-callees ... materialize_call_boundary_source_to_destination ...`;
`c4c-clang-tool-ccdb function-callees ... materialize_indirect_call_callee_to_prepared_register ...`;
`c4c-clang-tool-ccdb function-callees ... record_call_result_source_register ...`;
`rg` and `sed` source inspections; `ctest --test-dir build -N | rg -i
"call|aarch64|backend"` to identify candidate focused tests.
