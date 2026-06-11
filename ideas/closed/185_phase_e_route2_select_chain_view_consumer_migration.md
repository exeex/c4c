# 185 Phase E Route 2 select-chain view consumer migration

## Goal

Switch one select-chain or direct-global materialization reader to the Route 2
select-chain/direct-global view.

## Why This Exists

Phase D identified Route 2 facts as ready for a bounded selected-consumer
migration. The implementation should keep prepared select-chain and
direct-global helpers public while proving equivalence for one concrete
consumer path.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One AArch64 select materialization, direct-global dependency, scalar
  publication, call, memory, or ALU subpath that needs select-root facts.
- A Route 2 view returning select-root identity, root instruction index, scalar
  eligibility, and direct-global dependency presence.
- Prepared select-chain materialization and direct-global dependency helpers as
  public fallbacks.

## Out Of Scope

- Target materialization sequence choice, storage/home selection, memory
  operand formation, call-publication policy, or final record spelling.
- Deleting prepared select-chain/direct-global helpers.
- Migrating multiple consumer families in one slice.

## Acceptance Criteria

- The selected consumer uses the Route 2 view for semantic select-chain or
  direct-global facts.
- Tests prove route/prepared equivalence for direct-global present/absent,
  scalar-eligible/ineligible, and nested select-chain cases.
- Prepared oracle tests remain active and unweakened.

## Closure Notes

Closed after migrating the selected AArch64 scalar ALU control-publication
`select.result` path through `lower_scalar_select_publication(...)` to consult
the local Route 2 adapter first for select-root identity, root instruction
index, scalar eligibility, and direct-global dependency presence.

Prepared select-chain/direct-global helpers remain public fallback and oracle
surfaces. Remaining prepared consumers are future migration scope, not
unfinished work for this one-consumer idea.

Close proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract)$'`
passed in `test_after.log`, and the close-time regression guard passed against
`test_before.log` with 4/4 tests passing before and after.

## Reviewer Reject Signals

- The implementation encodes target materialization policy in the Route 2
  view.
- Route 2 absence is papered over with a broad scan or name match.
- The slice changes expectations instead of preserving behavior.
- One consumer migration is claimed as select-chain API contraction.
- Tests cover only a narrow named source pattern.
