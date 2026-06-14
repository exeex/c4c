Status: Active
Source Idea Path: ideas/open/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Selected LoadLocal Classification

# Current Packet

## Just Finished

Completed plan Step 2, `Selected LoadLocal Classification`.

Added opt-in prepared compare-join contract support for one selected
`EdgeStoreSlot` arm rooted in a same-predecessor-block `LoadLocal` with
authoritative local-slot addressing. The contract now has a `LocalI32Load`
computed base and return shape carrying predecessor label, load instruction
index, selected result name, slot identity, byte offset, size, alignment,
address space, and volatility. Source discovery remains fail-closed unless
the selected-loadlocal path is explicitly enabled, so existing x86 rendering
does not consume the new shape before Step 3.

Focused coverage was added in
`backend_x86_handoff_boundary_joined_branch_test.cpp` using a real BIR
predecessor-block `LoadLocal` incoming value. The tests prove the prepared
contract publishes the selected `LoadLocal`, rejects missing source-memory
authority, and does not classify a join-block carrier `LoadLocal` rewrite as
the selected predecessor source.

## Suggested Next

Execute Step 3 by wiring the new `LocalI32Load` compare-join return arm into
the x86 renderer through the existing Route 3 statement-memory agreement
facade. The renderer should explicitly opt into the selected-loadlocal
prepared contract, then fail closed on missing, incomplete, mismatched, or
unsupported Route 3/prepared source-memory authority.

## Watchouts

- `find_prepared_param_zero_materialized_compare_join_branches(...)` still
  defaults to the old empty-predecessor behavior. Step 3 must opt in to the
  selected-loadlocal contract at the x86 render site.
- `prepared_i32_compare_join_return_arm_is_supported(...)` currently returns
  false for `LocalI32Load` shapes; Step 3 owns changing that alongside the
  actual renderer branch.
- Keep using publication/source-memory facts from the real edge-transfer
  incoming value. Do not hand-build publications or infer from the join
  carrier load.

## Proof

Ran the delegated proof command:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` contains 2/2 default prepared tests and 2/2
x86 tests passing.
