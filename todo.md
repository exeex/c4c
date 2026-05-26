Status: Active
Source Idea Path: ideas/open/28_riscv_prepared_edge_publication_stack_source_policy_broadening.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Review Route Quality

# Current Packet

## Just Finished

Completed Step 2 by accepting shared lookup-backed RISC-V `StackSlot -> Register`
source homes with concrete `offset_bytes`, `size_bytes == 8`, and signed-12-bit
load offset, rendering `ld <dst>, <offset>(sp)`.

Preserved existing 4-byte stack-source `lw` behavior and existing
register/immediate/pointer-base/stack-destination consumers.

Added focused coverage for:

- positive 8-byte `ld` stack-source publication through shared
  `edge_publications`
- missing shared authority after clearing
  `publications_by_edge_destination`
- missing 8-byte offset
- unsupported subword and aggregate widths
- 8-byte signed-12-bit load offset overflow
- non-move 8-byte stack-source publications

## Suggested Next

Step 3 review completed with no blocking findings in
`review/idea28_riscv_8byte_stack_source_edge_publication_review.md`.
Supervisor should route the handoff/closure decision according to the active
plan.

## Watchouts

The new signed-12-bit offset guard is scoped to the newly accepted 8-byte stack
source form. Existing 4-byte `lw` stack-source behavior was left unchanged.
Sub-word, unsigned 32-bit, floating-point, dynamic-address, aggregate, and
large-offset 8-byte stack-source forms remain fail-closed.

## Proof

Passed.

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Proof log: `test_after.log`

Supervisor checks:

- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  passed with 5/5 before and 5/5 after.
- `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`
  passed 163/163 as broader backend validation.
