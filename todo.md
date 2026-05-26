Status: Active
Source Idea Path: ideas/open/19_x86_riscv_prepared_edge_publication_handoff.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Target-Neutral Proof

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: added the smallest focused x86 proof that a
non-AArch64 consumer reads shared prepared edge-publication facts through
`x86::ConsumedPlans::shared_function_lookups()->edge_publications`.

The proof extends the existing x86 prepared decoded-home fixture with one
prepared predecessor/successor join-transfer edge that matches an existing
block-entry parallel-copy move. The new x86-facing check uses
`find_unique_indexed_prepared_edge_publication` and
`find_unique_indexed_block_entry_parallel_copy_edge_publication` to consume the
shared publication and verifies semantic facts only: predecessor/successor edge
identity, source/destination prepared value ids, source/destination home kinds,
destination storage kind, block-entry phase, execution site, step ordering, and
links back to the shared prepared source records.

The x86 consumer proof keeps emission policy target-local: it does not assert
physical register names, scratch choices, instruction spelling, stack operand
syntax, clobber sequencing, or named-testcase shortcuts.

## Suggested Next

Proceed to Step 5 final handoff and validation. Summarize that x86 now has a
focused shared edge-publication consumption proof, while RISC-V remains a
readiness follow-up until it has a prepared-module lowerer entry point and
prepared function/block traversal surface.

## Watchouts

- The x86 proof intentionally consumes the indexed shared edge-publication
  authority rather than broadening `x86::prepared::Query` or adding a second
  target-specific wrapper.
- RISC-V still lacks a prepared-module lowerer entry point and prepared
  function/block traversal, so its first edge-publication consumer packet should
  wait for that integration surface.
- Scratch selection, clobber discipline, physical register spelling, stack
  operands, move instruction spelling, and assembly syntax remain target-local
  for both x86 and RISC-V.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_|backend_prepared_lookup|backend_prepare_liveness|backend_prealloc_block_entry_publications)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and 9/9 selected
tests passing.
