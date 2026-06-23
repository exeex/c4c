# RV64 Nested Select Chain Store Source Publication

## Goal

Repair RV64 prepared lowering for deeper nested select-chain store-source
publication when focused pointer-typed select publication succeeds but later
ternary joins still lack destination-access facts and truncate emitted code.

## Why This Exists

Idea 316 Step 5 evidence classified `src/00144.c` as a remaining nested
select-chain/store-source publication residual. The recent pointer-typed select
publication repair lets early pointer stores emit, but generated assembly later
truncates after nested select chains with repeated `mv t0, t0` and no `ret`.
Prepared store-source records show `missing_destination_access` at
`tern.end.23`, `tern.end.33`, `tern.end.54`, and `tern.end.63`.

## In Scope

- Nested select-chain publication into store sources on RV64.
- Prepared store-source destination-access facts for ternary join blocks.
- Pointer/integer select chains only where they feed the nested store-source
  publication path exposed by `src/00144.c`.
- Focused backend coverage for nested select-chain/store-source publication
  beyond the simple pointer-typed local publication already repaired.

## Out Of Scope

- Simple pointer-typed select publication into local pointer slots already
  repaired by idea 316.
- Stack-homed fused compare/control-flow emission for `src/00077.c` and
  `src/00143.c`.
- Generic local frame-slot address publication from idea 312.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow repair.

## Acceptance Criteria

- Focused tests cover nested select-chain store-source publication with
  destination-access facts for later ternary joins.
- `src/00144.c` either emits, links, and runs under qemu, or any remaining
  failure is reclassified with concrete emitted-code evidence as a different
  mechanism.
- Repairs consume or improve prepared store-source/select-chain facts rather
  than matching `tern.end.*` names, candidate filename, source spelling, or
  observed instruction text.
- Generated assembly no longer truncates with self-moves such as `mv t0, t0`
  hiding missing destination-access facts in the focused proof cases.

## Completion Note

Closed on 2026-06-23 after Step 3 prepared destination-access publication
repair satisfied the source idea criteria. Focused nested select store-source
dump coverage is now positive and forbids `missing_destination_access`; the
simple pointer-typed select publication neighbor remains green.

`src/00144.c` was reprobed under
`build/rv64_c_testsuite_probe_latest/triage_320_step3/` with BIR dump,
prepared-BIR dump, RV64 emit, clang link, and qemu all returning 0. Former
nested residual records including `tern.end.23`, `tern.end.33`, `tern.end.54`,
and `tern.end.63` now have destination access and report `status=available`;
`missing_destination_access` is absent from the prepared dump. No new RV64
residual was exposed by the reprobe.

Close gate: backend regression guard passed with
`test_before.log` versus `test_after.log` using
`--allow-non-decreasing-passed`: 275 passed, 1 failed before and after, with
the existing `backend_riscv_prepared_edge_publication` failure unchanged.

## Reviewer Reject Signals

- The implementation special-cases `src/00144.c`, `tern.end.23`, `tern.end.33`,
  `tern.end.54`, `tern.end.63`, or a fixed source ternary shape instead of
  repairing nested select-chain store-source publication.
- Progress is claimed by weakening qemu/runtime proof, marking the candidate
  unsupported, or changing expectations without a semantic publication repair.
- The patch only preserves the already repaired early pointer-typed select
  publication while later nested store-source records still have
  `missing_destination_access`.
- Target code adds local scans that duplicate prepared store-source authority
  without fixing the prepared destination-access gap.
- Broad switch/control-flow or ABI work is mixed into this route without
  focused nested select-chain/store-source proof.
