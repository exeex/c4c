Status: Active
Source Idea Path: ideas/open/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: x86 Render Bridge

# Current Packet

## Just Finished

Completed plan Step 3, `x86 Render Bridge`.

The x86 compare-join renderer now explicitly opts into the selected
`LoadLocal` prepared contract, treats `LocalI32Load` return shapes as
supported only with the new render branch, and renders the selected arm through
`render_agreed_route3_load_local_statement_memory_operand(...)`. The branch
revalidates the prepared predecessor block, instruction index, result id,
slot identity, byte offset, size, alignment, address space, and volatility
before loading from the agreed Route 3/prepared source-memory operand.

Focused x86 handoff coverage now proves the real `EdgeStoreSlot`
joined-branch selected-`LoadLocal` fixture emits through the bridge and rejects
an incomplete prepared source-memory publication instead of falling back past
the compare-join handoff.

## Suggested Next

Execute Step 4 by broadening the focused proof around the production bridge,
including route-debug coverage where applicable, and record whether idea 261
can resume its supported publication fixture work.

## Watchouts

- The Route 3 memory record stores the BIR local-slot-relative byte offset;
  prepared source-memory publications expose the rendered frame operand
  offset. The bridge therefore keeps byte-offset authority on the selected
  `LoadLocal` itself and uses publication availability, producer identity,
  slot ownership, size, alignment, address space, and volatility for the
  Route 3/prepared agreement check.
- Step 4 should add any remaining reachable negative rows without synthetic
  publications or fixture-only `join_transfers`.

## Proof

Ran the delegated proof command:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` contains 2/2 default prepared tests and 2/2
x86 tests passing.
