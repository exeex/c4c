Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun Representative And Route Next Boundary

# Current Packet

## Just Finished

Step 3: Lower Supported Same-Module Byval Aggregate Call Argument completed.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

RV64 same-module byval aggregate-address call lowering now consumes the
prepared `PreparedAggregateTransportKind::StackCopy` contract instead of
reconstructing payload layout from BIR text, testcase names, or stack-object
suffixes. The lowering validates the local frame-address source selection,
explicit source stack offset, byval ABI size/alignment, required payload chunks,
and exact non-overlapping copy coverage before allocating the outgoing byval
copy and placing its address in `a0`.

The required object-emission proof path uses
`src/backend/mir/riscv/codegen/object_emission.cpp`, so that related RV64
object-route dispatcher was updated in addition to the delegated
`prepared_call_emit.cpp` text path. Focused object coverage proves the accepted
same-module path and fail-closed rejection for missing aggregate transport,
unsupported aggregate transport kind, missing source stack offset, and
unsupported/fallback chunk kind.

## Suggested Next

Execute Step 4 from `plan.md`: rerun `src/20030914-2.c` through the RV64 object
route and decide whether the active idea is ready for broader validation or
needs another focused packet.

## Watchouts

- Do not reopen prepared global-symbol publication; idea 384 closed that
  boundary.
- Do not treat closed byval parameter-home idea 370 as owning this callsite
  movement boundary.
- Keep aggregate `va_arg` and non-register parameter-home work routed to ideas
  371 and 374 respectively.
- Do not modify or rely on unrelated EV64 idea 385 for this lifecycle route.
- Step 4 should rerun `src/20030914-2.c`; this packet did not run it.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` is the existing
  test file for the delegated `backend_riscv_object_emission` proof target,
  even though the packet named `tests/backend/bir` for optional focused tests.
- The object-route fixture currently uses the shared prepared stack-layout
  frame size, so the assertions account for the normal function prologue and
  epilogue around the call fragment.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_call_boundary_effect_plan|backend_riscv_object_emission)$'
```

Result: passed. Canonical proof log: `test_after.log`.
