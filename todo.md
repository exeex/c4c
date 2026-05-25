Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove One Residual Reconstruction Path

# Current Packet

## Just Finished

Step 2 removed the residual post-boundary-effect small byval stack-lane
reconstruction path in `calls_moves.cpp`. The deleted code no longer reads
retained `bir::CallInst::arg_abi` through
`prepared_argument_is_small_byval_stack_lane()`, and `lower_before_call_moves()`
now relies on prepared boundary effects/move bundles for
`"call_arg_byval_aggregate_register_lanes"` stack-lane moves.

## Suggested Next

Supervisor should select the next packet from the remaining Step 1 audit
families. A coherent next slice is the local aggregate address publication
predicate duplicate-authority family in `calls_argument_sources.cpp` and
`calls_dispatch_bridge.cpp`, if the supervisor wants to continue removing
retained call-argument ABI planning reads.

## Watchouts

- The outgoing stack-byte computation still appears to need a new prepared slot
  width fact before retained `arg_abi` can be removed there.
- The local aggregate address publication predicates in
  `calls_argument_sources.cpp` and `calls_dispatch_bridge.cpp` are a separate
  coherent duplicate-authority family; keep them out of the selected Step 2
  packet.
- Existing aggregate stack-lane lowering diagnostics remain in
  `lower_before_call_move()` when prepared source bytes or destination stack
  offsets are missing; this packet did not add fallback reconstruction.

## Proof

Supervisor-selected proof passed and is preserved in `test_after.log`:

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_call_boundary_owner|backend_aarch64_machine_printer|backend_call_boundary_effect_plan)$"; } > test_after.log 2>&1'`
