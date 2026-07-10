Status: Active
Source Idea Path: ideas/open/669_byval_prepared_dump_contract_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Byval Dump Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: refresh focused prepared-BIR dump evidence for
`backend_dump_riscv64_byval_aggregate_fixed_call` and
`backend_dump_riscv64_byval_preserved_pointer_args`, then record whether each
row is stale snippet expectation, dump text emission, or missing prepared
publication.

## Watchouts

- Treat route and runtime byval rows as regression surfaces, not the
  implementation target.
- Do not reopen the closed idea 659 byval runtime/codegen-route repair without
  fresh focused regression evidence.
- Do not work on object-runtime `BinaryInst`; that belongs to idea 670.
- Do not use testcase names, fixed value IDs, or final assembly shape as the
  dump-contract authority.

## Proof

Activation-only lifecycle change; no build or regression proof required.
