Status: Active
Source Idea Path: ideas/open/21_x86_prepared_edge_publication_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the Minimal x86 Consumer Path

# Current Packet

## Just Finished

Completed Step 1 mapping for the minimal x86 consumer path. The selected path is
`x86::consume_plans(prepared, function)` -> `ConsumedPlans::shared_function_lookups()`
-> `PreparedFunctionLookups::edge_publications`, consumed by a small x86-owned
edge-publication move helper before broader lowering wiring.

## Suggested Next

Implement Step 2 as a focused code packet in
`src/backend/mir/x86/x86.hpp`, `src/backend/mir/x86/prepared/prepared.hpp`,
`src/backend/mir/x86/prepared/dispatch.cpp`, and
`tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`. Add an
x86-owned helper that reads the exact shared lookup call site:
`prepare::find_unique_indexed_prepared_edge_publication(&lookups->edge_publications,
predecessor_label, successor_label, destination_value_id)`, where `lookups`
comes from `consumed.shared_function_lookups()`. The helper should return a
target-local move intent for the first proof: for the existing fixture edge
`entry -> join`, destination `PreparedValueId{5}`, source value `2` stack-slot
home at offset `56` publishes to destination register home `ebx`, producing an
x86-owned output shape equivalent to `mov ebx, DWORD PTR [rsp + 56]` or an
explicit structured move intent with those source/destination fields. Keep the
helper missing-authority aware when the shared lookup, source home, destination
home, or supported x86 operand form is absent.

## Watchouts

- Do not create x86-local edge-copy semantic authority.
- Do not move target-local emission policy into shared prepare.
- Do not weaken supported-path expectations.
- The current x86 fixture already proves read-only shared lookup access through
  `check_x86_consumed_plans_read_shared_edge_publications`; the next proof must
  fail if the new x86 helper ignores the shared `edge_publications` lookup or
  derives predecessor/successor/value facts locally.
- No blocker found in the mapping pass. The minimum surface exists through
  `ConsumedPlans::prepared_lookups`; broader `module.cpp` lowering wiring can
  wait until after the helper proof.

## Proof

Mapping-only packet; no build required and no root-level logs created.
Recommended next proof command:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_prepared_decoded_home_storage|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`.
