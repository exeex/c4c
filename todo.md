Status: Active
Source Idea Path: ideas/open/75_shared_aggregate_transport_plan_probe.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Shared Transport Authority If Required

# Current Packet

## Just Finished

Step 3 of `plan.md` repaired the AArch64 call-boundary owner byval
register-lane test after the lowering migration to
`PreparedCallArgumentPlan::aggregate_transport`.

- `backend_aarch64_call_boundary_owner_test.cpp` now publishes a
  `ByvalRegisterLanes` `PreparedAggregateTransportPlan` for the byval caller
  publication case, including the shared payload size/alignment, source frame
  slot/offset, required chunks, and destination lane bindings for `x0`/`x1`.
- The existing byval lane output contract remains intact: the test still
  expects one call-boundary move that materializes payload lanes from prepared
  source memory instead of forwarding the object pointer in `x20`.

## Suggested Next

Next coherent Step 3 packet: supervisor review of the aggregate-transport
consumer migration and proof logs before deciding whether Step 3 is ready for
broader review or closure.

## Watchouts

- The owner test now mirrors the shared transport facts required by lowering;
  keep this as a data contract repair, not a fallback relaxation.
- Keep variadic aggregate `va_arg` plans separate.
- Do not weaken aggregate testcase expectations or add named-case shortcuts if
  any remaining consumer migration is needed.

## Proof

Command:

```bash
cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' >> test_after.log
```

Result: passed, 4/4 selected tests. Proof log: `test_after.log`.
