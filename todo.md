# Current Packet

Status: Active
Source Idea Path: ideas/open/728_prepared_return_chain_shape_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Bind each failure to one common producer seam

## Just Finished

- Plan Step 3 bound both focused failures to the normal value-location producer
  `build_prepared_value_location_function`: after it converts
  `regalloc_function.move_resolution` through `append_prepared_move_bundle`, it
  calls only `append_prepared_call_abi_bindings`, so attributed `BeforeReturn`
  `FunctionReturnAbi` moves produced earlier by `append_return_move_resolution`
  have no terminal `PreparedAbiBinding` publication pass.
- This is the earliest exact common insertion point because
  `append_return_move_resolution` owns the regalloc move record but not the
  public move bundle or value-home collection, while
  `build_prepared_value_location_function` first owns the normalized terminal
  bundle, its function identity, and the matching prepared home together.
- Successor attribution, exact block/instruction position, adjacency, and
  traversal attachment do not need changes. The multi-link probe already proves
  the successor bundle at block 0 / instruction 1, and traversal reaches the
  terminal move in both shapes before returning `StructurallyIncomplete` solely
  because `terminal_binding == nullptr`.
- A generic return binding must be appended to the same attributed
  `BeforeReturn` bundle as each unique terminal move and copy its complete
  destination identity: `FunctionReturnAbi`, destination storage kind, optional
  ABI lane index, register name, contiguous width, occupied register names,
  stack offset, register placement, and target-register identity. Its source
  identity is the move's `from_value_id == to_value_id`; that id must resolve to
  exactly one complete `PreparedValueHome` whose `value_id` matches and whose
  `function_name` equals both the bundle and
  `PreparedValueLocationFunction::function_name`. Bundle
  `proof_attribution_id != 0`, `BeforeReturn` block/instruction placement, and
  the same function id supply freshness; no opcode, target, register spelling,
  or fixed-position inference is permitted. `PreparedAbiBinding` itself need
  not duplicate source/home/function fields because the terminal consumer binds
  it through the uniquely matched move inside that fresh function-owned bundle.

## Suggested Next

- Plan Step 4 packet: own only `src/backend/prealloc/regalloc.cpp`,
  `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`, the two
  focused return-chain probe cases, `todo.md`, and `test_after.log`. Add one
  target-independent function-return ABI-binding publication pass at
  `build_prepared_value_location_function`, preserve unique/fail-closed matching,
  and invert only the focused terminal-binding/status assertions to require the
  matching binding and `Available`; do not change external test targets or
  expectations. Prove with the same build plus committed 10-test direct-status
  subset used for Step 2.

## Watchouts

- Do not put the repair in `append_return_move_resolution`: that layer lacks the
  final normalized public bundle and prepared-home ownership needed to publish
  a fresh binding. Do not widen into successor reconstruction, traversal
  classification, call ABI consumers, or AArch64 code.
- Binding deduplication currently compares destination fields only. The Step 4
  pass must reject ambiguous terminal moves/homes rather than letting
  destination-only deduplication hide conflicting source ownership.

## Proof

- Audit-only packet; no test rerun required. Relied on the committed Step 2
  direct-status proof in `test_after.log`: the matching 10-test subset passed
  10/10 and directly observed both public shapes without external expectation
  changes.
- AST-backed inspection covered `append_return_move_resolution` and its direct
  callees, `build_prepared_value_location_function` and its direct callees,
  `append_prepared_call_abi_bindings`, `append_prepared_abi_binding`, the
  `PreparedAbiBinding`/`PreparedMoveBundle` fields, and the terminal-binding and
  successor branches in `classify_prepared_object_return_chain`.
