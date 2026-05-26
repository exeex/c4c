Status: Active
Source Idea Path: ideas/open/31_riscv_prepared_edge_publication_stack_source_policy_followup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Remaining Stack-Source Candidates

# Current Packet

## Just Finished

Activated `ideas/open/31_riscv_prepared_edge_publication_stack_source_policy_followup.md` into `plan.md` and initialized this execution-state file.

## Suggested Next

Start Step 1 by inventorying existing RISC-V prepared edge-publication `StackSlot -> Register` support, the remaining fail-closed stack-source forms, and the safest next candidate policy.

## Watchouts

- Shared prepared `edge_publications` must remain the only semantic authority for edge moves.
- Do not broaden pointer-base register-destination or source-to-stack destination policy in this plan.
- Preserve existing concrete-offset 4-byte `lw` and 8-byte `ld` support.
- Treat fixture-name matching, expectation-only changes, and predecessor/successor block scans as route failures.

## Proof

Lifecycle activation only. No implementation files, tests, build artifacts, or root proof logs were touched.
