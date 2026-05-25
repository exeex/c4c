Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 1 selected exactly one retained metadata authority leak for the next
implementation packet: `aarch64_indirect_register_byval_argument`.

Selected leak:

- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
  `aarch64_indirect_register_byval_argument` still reopens the source
  `bir::CallInst` and reads `call->arg_abi[argument.arg_index]` to rediscover
  AArch64 indirect register byval shape.
- Its retained `CallInst::arg_abi` read is currently used by
  `make_prior_preserved_call_argument_source` in
  `src/backend/mir/aarch64/codegen/calls_preservation.cpp` to suppress
  prior-preserved source reuse for the large indirect byval path.
- Do not select or re-add the already removed
  `aarch64_stack_byval_argument_size_bytes` or
  `aarch64_indirect_byval_argument_size_bytes` helpers.

Prepared-fact mapping:

- Existing prepared replacement fact is
  `prepared_indirect_byval_extent_bytes(context, move, argument, source_home)`.
- That predicate already requires the prepared move to be a before-call
  call-argument ABI register move, the prepared source home to be a stack slot
  with `size_bytes > 16`, the argument source encoding to be `FrameSlot`, the
  argument source value to match `move.from_value_id`, the source bank to be
  `Gpr` or `AggregateAddress`, and the destination bank to be `Gpr`.
- This is the right authority for the preservation guard because
  `make_prior_preserved_call_argument_source` already has `argument`, `move`,
  and `source_home`; no missing prepared fact was found for this use.

## Suggested Next

Implement the `aarch64_indirect_register_byval_argument` deletion packet.

Expected deletion path:

- Expose or move `prepared_indirect_byval_extent_bytes` so
  `calls_preservation.cpp` can use it without reading `bir::CallInst`.
- Replace the preservation guard
  `aarch64_indirect_register_byval_argument(context, argument, instruction_index)`
  with `source_home != nullptr &&
  prepared_indirect_byval_extent_bytes(context, move, argument, *source_home).has_value()`.
- Delete the `aarch64_indirect_register_byval_argument` declaration from
  `calls.hpp` and its definition from `calls_byval_aggregates.cpp`.
- Keep the implementation packet scoped to this one retained
  `CallInst::arg_abi` authority leak.

Focused proof command for that implementation packet:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  durable blockers remain.
- The stack byval fallback path is no longer a blocker; do not re-add
  `aarch64_stack_byval_argument_size_bytes`.
- The removed indirect byval size fallback path is no longer a blocker; do not
  re-add `aarch64_indirect_byval_argument_size_bytes`.
- `prepared_indirect_byval_extent_bytes` is the selected prepared authority for
  this packet; `prepared_byval_lane_extent_bytes` remains only for small
  `"call_arg_byval_aggregate_register_lanes"` payload-lane moves.
- The publication blockers in `calls_dispatch_bridge.cpp` and
  `calls_argument_sources.cpp` remain separate durable candidates.
- The `calls_dispatch_bridge.hpp` `CallInst`-shaped helper boundary remains a
  separate durable candidate.
- Treat retained `CallInst` reads as acceptable only for identity validation,
  diagnostics, or emission context, not for rederiving prepared call-plan
  decisions.

## Proof

Analysis evidence:

- `rg -n "arg_abi|arg_types|CallInst" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls*.hpp src/backend/mir/aarch64/codegen/calls.hpp`
- `rg -n "aarch64_indirect_byval_argument_size_bytes|prepared_indirect_byval_extent_bytes|aarch64_indirect_register_byval_argument" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls*.hpp src/backend/mir/aarch64/codegen/calls.hpp`

Selected todo-only packet proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed in `test_after.log`; 162/162 `^backend_` tests passed.
