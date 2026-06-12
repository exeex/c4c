Status: Active
Source Idea Path: ideas/open/211_route5_current_block_join_source_semantic_reader.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Fail-Closed Join-Source Behavior

# Current Packet

## Just Finished

Step 3 proved the selected
`current_block_join_prepared_query_source(...)` reader fails closed across the
Route 5 join-source cases the fixture can observe. The
`backend_aarch64_current_block_join_routing` fixture now includes a
`DuplicateSource` route shape with duplicate indexed Route 5 join records: the
direct Route 5 scan can still see the duplicated PHIs, but the indexed Route 5
identity used by the reader is unavailable, so the selected source reader keeps
the prepared source bit.

The same focused test continues to prove positive agreement, no prepared
policy, absent Route 5 source facts, missing/invalid named source evidence,
route/prepared source disagreement, memory-source disagreement, and prepared
fallback for the selected reader. The adjacent
`current_block_join_prepared_query_incoming_expression(...)` reader was not
migrated or weakened.

## Suggested Next

Execute Step 4 by proving joined-branch, prepared-output, and wrapper output
surfaces remain byte-stable for the selected Route 5 reader migration. Keep the
packet to proof and only add assertions if a byte-stability surface is missing.

## Watchouts

- Do not move branch, parallel-copy, execution-site, value-home, move-bundle,
  output, wrapper, or expected-string policy into Route 5.
- Do not rewrite expected strings as Step 4 proof; use existing byte-stable
  surfaces unless a separate approved semantic change requires otherwise.
- Duplicate/conflicting Route 5 join evidence is observable at this reader via
  the indexed Route 5 path becoming unavailable while prepared fallback remains
  active.
- If x86 backend tests are enabled in a later build, add
  `backend_x86_handoff_boundary_test` and `backend_x86_handoff_boundary` to the
  final wrapper proof; they are not available in the current configured CTest
  set.

## Proof

Passed the exact delegated proof command. `test_after.log` records 5/5 passing:

```bash
cmake --build --preset default --target backend_aarch64_current_block_join_routing_test backend_aarch64_instruction_dispatch_test backend_prepared_lookup_helper_test backend_prepared_printer_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_current_block_join_routing|backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper|backend_prepared_printer|backend_riscv_prepared_edge_publication)$' > test_after.log
```
