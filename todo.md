Status: Active
Source Idea Path: ideas/open/33_riscv_prepared_edge_publication_remaining_stack_destination_sources.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Remaining Stack-Destination Candidates

# Current Packet

## Just Finished

Activated `ideas/open/33_riscv_prepared_edge_publication_remaining_stack_destination_sources.md` into `plan.md` and initialized this execution-state file.

## Suggested Next

Start Step 1 by inventorying existing RISC-V prepared edge-publication source-to-`StackSlot` support, the remaining fail-closed forms, and the safest next candidate policy.

## Watchouts

- Shared prepared `edge_publications` must remain the only semantic authority for edge moves.
- Do not broaden stack-source or pointer-base register-destination policy in this plan.
- Preserve existing `Register -> StackSlot` and `RematerializableImmediate -> StackSlot` I32 support.
- Treat fixture-name matching, expectation-only changes, predecessor/successor block scans, and target-neutral policy movement as route failures.

## Proof

Lifecycle activation only. No implementation files, tests, build artifacts, source ideas, closed ideas, or root proof logs were touched.
