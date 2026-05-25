Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Affected Helper Boundary

# Current Packet

## Just Finished

Step 3 completed the helper-boundary closeout for the selected indirect byval
size path.

Completed work:

- Verified no surviving `aarch64_indirect_byval_argument_size_bytes`
  declaration, definition, source call site, or test call site remains under
  `src` or `tests`.
- Verified `prepared_indirect_byval_extent_bytes` remains a local
  `calls_moves.cpp` emission helper with one local call site.
- Confirmed `prepared_indirect_byval_extent_bytes` reads only prepared
  context/move/argument/home facts and does not read retained `bir::CallInst`,
  `CallInst::arg_abi`, or `CallInst::arg_types`.
- No source edits were needed for this closeout packet.

## Suggested Next

Step 4 should run the broader backend checkpoint for the completed selected
indirect byval size path and hand results to the supervisor for commit
selection.

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  these durable blockers remain.
- The stack byval fallback path is no longer a blocker; do not re-add
  `aarch64_stack_byval_argument_size_bytes`.
- This selection still does not cover `aarch64_indirect_register_byval_argument`;
  that remains a separate retained `CallInst::arg_abi` shape predicate after
  the selected size fallback removal.
- Treat retained `CallInst` reads as acceptable only for identity validation,
  diagnostics, or emission context, not for rederiving prepared call-plan
  decisions.
- The indirect byval fallback intentionally remains separate from
  `prepared_byval_lane_extent_bytes`, which is only for small
  `"call_arg_byval_aggregate_register_lanes"` payload-lane moves.

## Proof

Required Step 3 proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address)$') > test_after.log 2>&1`

Result: passed 3/3 selected tests; proof log preserved at `test_after.log`.
