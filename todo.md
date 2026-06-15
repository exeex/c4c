Status: Active
Source Idea Path: ideas/open/280_phase_f5_edge_publications_prepared_only_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve The Compatible Positive Path

# Current Packet

## Just Finished

Completed `plan.md` Step 3 as a todo-only status update by recording the
compatible positive path preserved by the committed Step 2 proof. The same real
RISC-V dynamic `LoadLocal` consumer path calls
`riscv::consume_edge_publication_move_intent(..., &route5_memory_edge)` for the
public `left -> join` prepared publication with destination value id `2` /
`%dst`, source value id `1` / `%src`, source memory `%base + 12`, size `4`, and
destination register `a1`.

The committed assertion keeps the compatible Route 5 authority as
`MemorySource`, returns `Available`, and requires exact output
`lw a1, 12(s2)`. This preserves the agreeing dynamic stack-source path while
the Step 2 prepared-only `NoSource` row remains fail-closed.

## Suggested Next

Execute Step 4: run the supervisor-selected focused acceptance proof for the
prepared-only `edge_publications` fail-closed row and preserve `test_after.log`.

## Watchouts

- The prepared-only proof uses the real RISC-V consumer path and a Route 5
  `NoSource` record, not helper-only lookup rejection or synthetic public
  lookup mutation.
- The positive path is already asserted through the same dynamic `LoadLocal`
  row with matching Route 5 `MemorySource` authority and exact
  `lw a1, 12(s2)` output.
- Use the dynamic `LoadLocal` row for this source idea; scalar register rows
  still preserve prepared fallback on non-agreeing Route 5 facts.
- Do not weaken fallback, status/debug, route-debug, prepared-printer,
  helper/oracle, wrapper, exact-output, or baseline contracts.
- Keep this runbook to one bounded prepared-only `edge_publications` row.

## Proof

Todo-only packet. No build or tests run, and no logs touched. Positive-path
evidence comes from the already committed Step 2 assertion in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` that the
compatible dynamic `LoadLocal` path with `route5_memory_edge` returns
`Available` and emits exact `lw a1, 12(s2)`.
