Status: Active
Source Idea Path: ideas/open/275_riscv_memory_accesses_stale_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run Acceptance Proof

# Current Packet

## Just Finished

Step 4 - Run Acceptance Proof completed.
Supervisor reran the focused acceptance command after Step 3:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' | tee test_after.log`.
The focused backend test passed 1/1. The matching before/after regression guard
also passed with `--allow-non-decreasing-passed`: no new failures, no failed
tests, and no suspicious runtime additions.

The accepted fixture coverage remains:
`check_load_local_dynamic_stack_source_exposes_shared_memory_access()` asserts
the compatible LoadLocal path through normal prepared lookups, including the
unique public source memory row, the selected current memory access, and the
rendered `lw a1, 12(s2)` instruction. The same test constructs the stale fixture
through normal `PreparedAddressingFunction::accesses` input, asserts that both
current and stale rows are published by source value id, verifies the stale row
keeps offset `16`, and checks RISC-V rejects that stale public lookup without
rendering an instruction.

## Suggested Next

Ask the plan owner to close the active plan if the source idea is complete.

## Watchouts

- This runbook intentionally did not add the later full stale-publication
  fail-closed proof beyond fixture support.
- No backend/prealloc implementation files were changed.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' | tee test_after.log`
passed. Regression guard against `test_before.log` passed with
`--allow-non-decreasing-passed`.
