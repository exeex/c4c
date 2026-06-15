Status: Active
Source Idea Path: ideas/open/276_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve The Compatible Positive Path

# Current Packet

## Just Finished

Step 3 - Preserve The Compatible Positive Path is complete as a todo-only
status update.
The committed Step 2 proof already preserves the compatible dynamic
stack-source `LoadLocal` path in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`: the
positive path still asserts `EdgePublicationMoveIntentStatus::Available`, exact
`instruction_text == "lw a1, 12(s2)"`, `route5_edge_status ==
bir::Route5PublicationStatus::MemorySource`, `route5_edge_source_agrees`, and
`route3_source_memory_agrees`. This keeps the current Route 3 / Route 5
agreement path supported while the stale public `memory_accesses` row is
rejected fail-closed.

## Suggested Next

Start Step 4 by running the supervisor-selected acceptance proof for the
preserved compatible path plus stale-publication fail-closed coverage.

## Watchouts

- Keep cross-publication, byte-offset coverage, x86 parity, edge-publication
  families, metadata, liveness, aggregate retirement, and draft 155 work out
  of this runbook.
- The stale proof is intentionally fixture-backed and uses no synthetic
  post-construction mutation, named-case shortcut, or expectation downgrade.
- Route 3 / Route 5 authority must stay on the current offset `12` row; moving
  it to the stale public offset `16` only proves a weaker mismatch case.
- Step 3 made no code changes; it records that the positive assertion is
  already covered by the Step 2 test diff.

## Proof

Todo-only packet. No build or tests were run, and no logs were touched.
