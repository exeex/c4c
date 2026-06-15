Status: Active
Source Idea Path: ideas/open/275_riscv_memory_accesses_stale_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Compatible And Stale Fixture Coverage

# Current Packet

## Just Finished

Step 3 - Prove Compatible And Stale Fixture Coverage completed.
No code changes were needed: Step 2 coverage already satisfies this packet.
`check_load_local_dynamic_stack_source_exposes_shared_memory_access()` asserts
the compatible LoadLocal path through normal prepared lookups, including the
unique public source memory row, the selected current memory access, and the
rendered `lw a1, 12(s2)` instruction. The same test also constructs the stale
fixture through normal `PreparedAddressingFunction::accesses` input, asserts
that both current and stale rows are published by source value id, verifies the
stale row keeps offset `16`, and checks RISC-V rejects that stale public lookup
without rendering an instruction.

## Suggested Next

Start the next supervisor-selected packet; Step 3 fixture coverage is proven.

## Watchouts

- Step 3 did not add the final fail-closed proof beyond fixture coverage; the
  delegated packet explicitly stopped before that broader proof.
- No backend/prealloc implementation files were changed.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' | tee test_after.log`
passed. Proof log: `test_after.log`.
