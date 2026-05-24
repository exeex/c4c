Status: Active
Source Idea Path: ideas/open/target-neutral-publication-plan-record.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Cross-Target Reuse Proof

# Current Packet

## Just Finished

Completed Step 4: `Add Cross-Target Reuse Proof`.

Added `backend_x86_publication_plan_reuse`, an x86-labeled MIR backend test
that consumes `prepare::plan_prepared_scalar_publication` and
`prepare::PreparedScalarPublicationPlan` without AArch64 codegen headers,
register views, memory operands, mnemonics, scratch indexes, or machine
instruction records.

The test exercises real record behavior through a tiny x86-facing adapter:
register-home, stack-slot, rematerializable-immediate, and
pointer-base-plus-offset plans map to distinct reuse actions, while incomplete
missing-source, missing-slot, and missing-delta records are rejected from their
neutral status fields.

## Suggested Next

Ask plan-owner to decide whether the active target-neutral publication plan is
complete enough to close or whether a narrow follow-up plan is needed for the
remaining target-policy-adjacent publication cases.

## Watchouts

This proof is intentionally record-level and target-adapter-level; it does not
claim that x86 lowering implements prepared scalar publication. Remaining
AArch64 direct publication fallbacks still involve reload policy, immediate
materialization, pointer-address construction, global-load sequences, scratch
choice, register alias checks, or instruction construction and should stay local
unless a later plan defines a tighter neutral boundary.

## Proof

Ran supervisor-selected proof:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: build succeeded and `158/158` backend tests passed, `0` failed.
Proof log: `test_after.log`.
