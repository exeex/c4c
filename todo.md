Status: Active
Source Idea Path: ideas/open/278_phase_f5_memory_accesses_byte_offset_coverage_gate.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Record Coverage Remainder

# Current Packet

## Just Finished

Completed Step 5 of `plan.md` by recording the coverage remainder for the
byte-offset gate.

Proven row: the RISC-V same-consumer public lookup path now has a same-block
duplicate `PreparedMemoryAccess` row for the same `%src` value at byte offset
`16`, while the selected publication, selected position row, and Route 3 /
Route 5 authority remain at byte offset `12`. The real
`consume_edge_publication_move_intent(..., &route5_memory_edge)` consumer fails
closed as `UnsupportedSourceHome`, emits no instruction, records no
source-memory offset, and keeps Route 5 / Route 3 agreement true for the
selected offset `12` row. The compatible dynamic stack-source `LoadLocal` path
still emits exact `lw a1, 12(s2)`.

## Suggested Next

Supervisor should decide the next lifecycle action for this active plan using
the Step 5 remainder notes.

## Watchouts

Do not claim exhaustive byte-offset completion from this gate. Remaining
unsupported or unexamined byte-offset surfaces include:

- Shared source-producer planner consumers using result-value-name or
  positional `find_indexed_prepared_memory_access` paths.
- AArch64 target operand, ALU, and memory operand consumers of prepared-memory
  lookup data by result value id or positional access.
- Wrapper-level negative exact output for a same-block duplicate public row.
- Synthetic prepared offset drift between
  `publication.source_memory_byte_offset`, `publication.source_memory_access`,
  and alternate public lookup entries with the same source value.

No fixture-support blocker remains for the proven RISC-V same-block public
lookup row. Separate fixture-support work may be needed only if the supervisor
opens wrapper-level negative-output or target-operand byte-offset packets.

## Proof

Todo-only packet per supervisor delegation. No build or tests were run, and
logs were not touched.
