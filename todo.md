Status: Active
Source Idea Path: ideas/open/382_rv64_object_route_prepared_local_memory_addressing.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Supported And Rejected Fixtures

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding focused RV64 object-emission fixtures for
the audited byval stack-slot pointer-value local-memory access boundary.

`tests/backend/mir/backend_riscv_object_emission_test.cpp` now parameterizes the
existing `make_prepared_byval_stack_slot_param_module` access offset and asserts
the supported 4-byte lane load shape at both offset `0` and the final in-bounds
offset `68`. The supported expectations check the exact generated RV64 text:
frame allocation, `lw a0, 0(sp)` for offset `0` or `lw a0, 68(sp)` for offset
`68`, frame release, `ret`, and no relocations.

The adjacent fail-closed byval pointer-access fixtures now cover missing
pointer identity, missing byval/home facts that are rejected by the earlier
parameter-home checks, non-pointer/non-base-plus-offset addressing,
out-of-bounds offset `69`, non-default address space, volatile access,
over-alignment, and an unsupported `I128` local-memory load size.

## Suggested Next

Next packet should move to the next supervisor-selected plan step. If Step 3 is
lowering work, use these fixtures as the acceptance boundary and keep the
supported byval shape semantic rather than testcase-name based.

## Watchouts

- A fabricated unencodable byval home offset was not retained because it trips
  the supported-stack-frame admission diagnostic before the local-memory
  boundary. The feasible local fixture covers the adjacent out-of-bounds offset
  `69` instead.
- The current checkout already lowers the supported byval stack-slot pointer
  lane loads; the new supported expectations are green here rather than acting
  as red tests.
- Missing byval/home facts can be rejected before local-memory analysis by
  `unsupported_param_home` or `unsupported_byval_param_home`; those expectations
  are intentionally precise.

## Proof

Ran the delegated proof command exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1
```

Result: passed. Build relinked `backend_riscv_object_emission_test`, and CTest
reported `1/1 Test #215: backend_riscv_object_emission .... Passed`. Proof log:
`test_after.log`.
