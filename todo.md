Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Residual Prepared-Fact Gaps

# Current Packet

## Just Finished

Step 1 audited residual prepared-fact gaps in the AArch64 calls helper family
using `c4c-clang-tool-ccdb` symbol, `CallInst` type-ref, caller, and callee
queries plus focused line inspection of the named files.

Retained reads found and classified:

- `calls_common.cpp:46-57` reads `bir::CallInst` and `call.arg_abi` in
  `outgoing_stack_argument_bytes()` to compute final stack reservation width
  from `PreparedCallPlan::arguments` offsets. Classification: missing
  prepared-fact dependency, because prepared arguments carry destination stack
  offsets but not the per-argument outgoing slot width needed to compute the
  reservation without retained ABI data.
- `calls_common.cpp:85-92` checks whether any instruction is a
  `bir::CallInst` in `function_has_call()`. Classification: emission
  validation/frame-shape query, not argument planning authority.
- `calls_argument_sources.cpp:465-509` reads retained call args,
  `arg_types`, and `arg_abi.byval_copy` to decide whether local frame address
  publication is legal for a pointer non-byval argument. Classification:
  duplicate planning authority; this predicate should consume prepared
  argument/source facts or stop on a missing prepared pointer/byval flag.
- `calls_byval_aggregates.cpp:226-249` reads retained `arg_abi` for
  byval register-lane size validation on prepared move records. Classification:
  duplicate planning authority with a diagnostic fallback shape.
- `calls_byval_aggregates.cpp:568-668` reads retained `arg_abi` in
  `aarch64_indirect_byval_argument_size_bytes()`,
  `aarch64_stack_byval_argument_size_bytes()`,
  `aarch64_register_byval_argument_size_bytes()`, and
  `aarch64_indirect_register_byval_argument()`. Classification: duplicate
  planning authority; these helpers rederive byval form/size/class from BIR
  instead of prepared call arguments and prepared move/binding facts.
- `calls_moves.cpp:2958-2980` reads retained `arg_abi` in
  `prepared_argument_is_small_byval_stack_lane()`. Classification: duplicate
  planning authority.
- `calls_moves.cpp:3029-3038` reads retained `bir::CallInst` so
  `lower_before_call_moves()` can call `outgoing_stack_argument_bytes()`.
  Classification: missing prepared-fact dependency, same stack reservation
  width issue as `calls_common.cpp`.
- `calls_moves.cpp:3052-3090` synthesizes missing byval stack-lane
  `PreparedMoveResolution` records from `PreparedCallPlan::arguments` after
  prepared boundary effects have already been planned. Classification:
  duplicate planning authority and selected Step 2 family.
- `calls_preservation.cpp:359-365` and `calls_preservation.cpp:387-392`
  stop use scans at the next `bir::CallInst`. Classification: emission
  validation/control-flow scan boundary, not argument ABI planning authority.
- `calls_dispatch_bridge.cpp:80-96` reads retained call args, `arg_types`, and
  `arg_abi.byval_copy` for local aggregate address publication eligibility.
  Classification: duplicate planning authority, same family as
  `calls_argument_sources.cpp` but in the dispatch bridge.
- `calls_dispatch_bridge.cpp:225-244` accepts a retained `bir::CallInst` for
  scalar argument producer lowering and delegates the local aggregate predicate.
  Classification: duplicate planning authority only through the predicate
  above; otherwise bridge/emission traversal.
- `calls_dispatch_bridge.cpp:259-285` accepts the retained call to match the
  prepared plan and handle dynamic/variadic dispatch. Classification:
  diagnostic identity/emission dispatch.
- `calls_dispatch_bridge.cpp:331-362` reads the current retained call args to
  map a prepared value name back to a BIR value when no prior instruction
  produced it. Classification: diagnostic/source identity fallback, not ABI
  planning.
- `calls_dispatch_bridge.cpp:709-735` reads retained indirect callee fields
  while validating the prepared indirect callee plan. Classification:
  emission validation/diagnostic identity.

## Suggested Next

Supervisor should delegate Step 2 for exactly one family: remove the
`calls_moves.cpp:3052-3090` small byval stack-lane synthetic fallback. The code
packet should make prepared boundary effects/move bundles the only authority
for `"call_arg_byval_aggregate_register_lanes"` stack-lane moves, delete or
bypass `prepared_argument_is_small_byval_stack_lane()`, and keep diagnostics in
the existing byval-lane lowering path when prepared facts are missing.

Expected deletion/move path: delete the post-boundary-effect synthetic
`PreparedMoveResolution` construction in `lower_before_call_moves()`, then
remove `prepared_argument_is_small_byval_stack_lane()` if no longer referenced.
Do not broaden into the local aggregate address publication predicate or the
outgoing stack reservation width gap in the same packet.

Focused proof command for that Step 2 packet:

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_call_boundary_owner|backend_aarch64_machine_printer|backend_call_boundary_effect_plan)$"; } > test_after.log 2>&1'`

## Watchouts

- Do not close this idea until the remaining AArch64 call helper surface and
  retained BIR metadata reads have been reviewed against prepared call-plan
  authority.
- Do not route broad dispatch cleanup into this plan; `calls_dispatch_bridge.*`
  is in scope only for call-boundary ownership.
- If a required fact is absent from prepared data, record that as a blocker
  instead of rebuilding call planning locally in AArch64 emission.
- The outgoing stack-byte computation still appears to need a new prepared slot
  width fact before retained `arg_abi` can be removed there.
- The local aggregate address publication predicates in
  `calls_argument_sources.cpp` and `calls_dispatch_bridge.cpp` are a separate
  coherent duplicate-authority family; keep them out of the selected Step 2
  packet.
- `calls_preservation.cpp` retained `CallInst` checks are scan boundaries for
  non-call-use analysis, not call-argument ABI planning.

## Proof

Audit-only packet. Supervisor-selected proof:

`git diff --check`
