Status: Active
Source Idea Path: ideas/open/276_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Stale Rejection Proof

# Current Packet

## Just Finished

Step 2 - Add The Stale Rejection Proof is complete.
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` now drives
the supported stale publication fixture through the real RISC-V same-consumer
backend path:
`consume_edge_publication_move_intent(&lookups, stale_ids.predecessor, stale_ids.successor, 2, &route5_memory_edge)`.
The repaired block keeps the prepared publication selected on the current
predecessor `LoadLocal` row at offset `12`, keeps the stale public
`memory_accesses` row visible at successor `join` / offset `16`, and proves the
public lookup publishes both rows for source value id `1`. It then builds the
Route 3 / Route 5 `MemorySource` authority from the current offset `12` row and
proves the same-consumer RISC-V path fails closed as
`EdgePublicationMoveIntentStatus::UnsupportedSourceHome` with empty
`instruction_text`, no `source_memory_byte_offset`, `route5_edge_status ==
MemorySource`, `route5_edge_source_agrees`, and
`route3_source_memory_agrees`. The stale public row is therefore the causal
rendering blocker, not a Route 5 mismatch.
The existing compatible dynamic stack-source path still proves
`lw a1, 12(s2)`.

## Suggested Next

Start Step 3 by adding the next planned stale-publication coverage slice only
if the supervisor confirms this RISC-V same-consumer proof is the desired
commit boundary.

## Watchouts

- Keep cross-publication, byte-offset coverage, x86 parity, edge-publication
  families, metadata, liveness, aggregate retirement, and draft 155 work out
  of this runbook.
- The stale proof is intentionally fixture-backed and uses no synthetic
  post-construction mutation, named-case shortcut, or expectation downgrade.
- Route 3 / Route 5 authority must stay on the current offset `12` row; moving
  it to the stale public offset `16` only proves a weaker mismatch case.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
