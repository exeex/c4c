Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handle Remaining Ready Families Or Split Dependencies

# Current Packet

## Just Finished

Completed `plan.md` Step 5 inventory for the remaining barrier/cache/
pause-hint/builtin-address intrinsic families.

- No remaining family is ready for AArch64 intrinsic machine-node selection or
  printing from current structured intrinsic carrier authority.
- BIR intrinsic families currently cover scalar FP unary, CRC, vector memory,
  and vector operation only; there are no barrier, cache-maintenance,
  pause/hint, or builtin-address `IntrinsicFamilyKind`/`IntrinsicOperationKind`
  facts to select from.
- Prepared intrinsic carrier validation only completes scalar `FAbs`, `Crc32W`,
  `VectorLoad`, and `VectorAdd`; all other intrinsic-family-shaped calls remain
  missing or unsupported rather than complete carriers.
- Existing atomic fence and prepared address materialization routes are
  separate structured carrier systems. They do not provide barrier or
  builtin-address intrinsic-family authority for this runbook.
- Implementing Step 5 selected records from intrinsic spelling alone would be
  route drift, so this packet did not touch implementation files or tests.

## Suggested Next

Request a plan-owner Step 6 lifecycle decision for source idea 239: either
close the current CRC/vector/scalar machine-node route as complete with
explicit dependency blockers, or split follow-up dependency ideas for
barrier/cache/pause-hint/builtin-address carrier authority before any machine
selection or printer work.

## Watchouts

- CRC/vector printer support is intentionally limited to selected structured
  records; do not widen it to intrinsic spelling alone, generic call plans,
  archived scratch registers, or unselected payloads.
- Vector load currently accepts the selected zero-offset pointer form used by
  the carrier path; offset/post-index forms need their own authority before
  being printed.
- Barrier/cache/pause-hint/builtin-address work is blocked on upstream BIR
  semantic intrinsic facts plus prepared intrinsic carriers; adding name-only
  records for these families would be route drift.
- Do not treat atomic fence carriers as barrier-intrinsic support or prepared
  address materialization records as builtin-address intrinsic support without
  an explicit lifecycle split that defines that dependency route.
- Keep unsupported x86-only and incomplete-fact paths fail-closed.

## Proof

Passed delegated proof; output preserved at `test_after.log`.

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_machine_printer|aarch64_instruction_dispatch|aarch64_target_instruction_records|prepared_printer|lir_to_bir_notes)'; } 2>&1 | tee test_after.log`

`git diff --check` passed.
