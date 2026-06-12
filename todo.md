Status: Active
Source Idea Path: ideas/open/211_route5_current_block_join_source_semantic_reader.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Route/Prepared Agreement for the Reader

# Current Packet

## Just Finished

Step 2 added Route 5/prepared agreement gating for the selected
`current_block_join_prepared_query_source(...)` reader only.
`build_current_block_join_prepared_query_routing(...)` now keeps the prepared
source bit as fallback and accepts Route 5 source evidence only when the Route 5
fact names the same current block, predecessor edge, source-producing
instruction/result, destination value, and prepared source-value relationship.

The adjacent
`current_block_join_prepared_query_incoming_expression(...)` reader was not
migrated. It continues to read the existing Route 5 incoming-expression bits
when Route 5 identity is available, including when prepared source facts are
unavailable.

Focused coverage updated
`backend_aarch64_current_block_join_routing` so:

- Route 5 without prepared policy no longer marks the selected source reader.
- A Route 5/prepared source disagreement retains the prepared source bit.
- Memory-source Route 5 evidence remains visible to the incoming-expression
  reader but is rejected by the selected source reader when the prepared
  destination/source relationship does not match.

## Suggested Next

Execute Step 3 by proving the fail-closed Route 5 join-source contract for the
selected reader. Keep the packet focused on positive agreement, no Route 5
source, invalid/missing source evidence, duplicate/conflicting indexed Route 5
records, route/prepared disagreement, and prepared fallback.

## Watchouts

- Keep the next packet to the selected source reader; do not migrate the
  adjacent incoming-expression reader.
- Do not move branch, parallel-copy, execution-site, value-home, move-bundle,
  output, wrapper, or expected-string policy into Route 5.
- The helper intentionally compares prepared label/value identities through the
  prepared name tables because Route 5 BIR identities and prepared IDs come
  from different name tables.
- Memory-source Route 5 records can be `Available` while still failing the
  selected reader's prepared destination/source agreement; this is expected
  fail-closed behavior.
- If x86 backend tests are enabled in a later build, add
  `backend_x86_handoff_boundary_test` and `backend_x86_handoff_boundary` to the
  final wrapper proof; they are not available in the current configured CTest
  set.

## Proof

Passed the exact delegated proof command. `test_after.log` records 5/5 passing:

```bash
cmake --build --preset default --target backend_aarch64_current_block_join_routing_test backend_aarch64_instruction_dispatch_test backend_prepared_lookup_helper_test backend_prepared_printer_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_current_block_join_routing|backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper|backend_prepared_printer|backend_riscv_prepared_edge_publication)$' > test_after.log
```
