# Route 3 And Diagnostics Planning Review

## Scope

Review window: `55125bd832a1a14b7c13cd7d954cd21c5a373875..521cf148fa7660766f5bfa8c0932833bfa987311`.

This is the window from the parent of idea 193 activation through closure of
idea 194. It covers 13 commits:

- `e58237c1f` activates idea 193.
- `9732ff7c8` closes idea 193.
- `f281426eb` activates idea 194.
- `521cf148f` closes idea 194.

Source ideas reviewed:

- `ideas/closed/193_route3_prepared_policy_boundary_hardening.md`
- `ideas/closed/194_prepared_printer_debug_oracle_replacement_planning.md`

Primary files reviewed:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- lifecycle movement for ideas 193 and 194

## Findings

No blocking findings.

### Non-Blocking Observation: Tests Prove The Selected Boundary, Not General Route 3 Retirement

The implementation in `src/backend/mir/aarch64/codegen/calls.cpp:8022`
introduces a local `IndirectCalleeStoredValueSource` classification and splits
Route 3 source identity from prepared fallback. The Route 3 path at
`src/backend/mir/aarch64/codegen/calls.cpp:8034` only reads
`find_bir_same_block_load_local_stored_value_source_identity(...)`; the fallback
path at `src/backend/mir/aarch64/codegen/calls.cpp:8064` keeps using
`prepare::find_prepared_same_block_load_local_stored_value_source(...)`.
The wrapper at `src/backend/mir/aarch64/codegen/calls.cpp:8098` explicitly
states that absent, mismatched, ambiguous, or policy-sensitive cases stay on
prepared fallback.

The tests in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:30695` and
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:30739` cover
the intended positive Route 3 identity path and the negative prepared-fallback
path. The positive fixture deliberately mismatches the prepared store slot while
leaving BIR memory identity intact; the fallback fixture erases BIR memory range
authority while keeping prepared policy usable. That is aligned with idea 193's
acceptance criteria. It should not be interpreted as general prepared Route 3
oracle retirement.

## Alignment Checks

Idea alignment: `matches source idea`.

The Route 3 slice hardens one selected indirect-callee stored-value-source
consumer. It does not move address materialization, frame layout, relocation,
storage selection, or final operand policy into Route 3 facts. It keeps the
prepared direct-global/select-chain and target emission decisions in the
existing AArch64 path at `src/backend/mir/aarch64/codegen/calls.cpp:8311`.

The diagnostics/oracle artifact matches idea 194. It is explicitly a planning
artifact at `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md:3`.
It states that prepared surfaces remain compatibility oracles until route-native
diagnostics and equivalent coverage exist at
`docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md:5`,
rejects production greenness and label rewrites as readiness evidence at
`docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md:24`,
and lists reject signals against expectation weakening and target-policy leakage
at `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md:116`.

Runbook-transcription judgment: `plan matches idea`.

The lifecycle sequence is coherent: idea 193 is activated, implemented, proven,
summarized, and closed before idea 194 is activated; idea 194 is then completed
as a docs/lifecycle planning artifact and closed. No active `plan.md` or
`todo.md` remains after `521cf148f`, and ideas 195-198 are still open for future
activation.

Route-alignment judgment: `on track`.

No testcase-overfit route failure was found. The code change is scoped to a
semantic same-block stored-value identity read plus prepared fallback. The new
tests add nearby positive and fallback coverage rather than weakening an
existing supported expectation.

Technical-debt judgment: `acceptable`.

The new helper names are local and somewhat specific to indirect callees, but
that is appropriate for idea 193's selected consumer. The `kind` field is not
currently consumed beyond classification, but it documents the boundary and may
support future diagnostics. No broader abstraction is required in this slice.

Target-policy leakage: none found.

The Route 3 code reads only target-neutral BIR source identity. Prepared
fallback remains responsible for addressing/fallback policy, and final register
materialization remains in the existing AArch64 callee materialization path.

Expectation weakening: none found.

The reviewed code/test/doc window adds tests and a planning document. It does
not delete, mark unsupported, rename, or weaken prepared diagnostic/oracle,
route-debug, wrapper, baseline, or c_testsuite expectations.

Prepared contraction readiness: not claimed.

The idea 194 artifact repeatedly states retained-oracle status and defines
future contraction prerequisites at
`docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md:138`.
It does not claim draft 155 readiness, `PreparedBirModule` demotion readiness,
or public prepared-surface deletion readiness.

## Validation

Validation sufficiency: `narrow proof sufficient`.

Implementation proof exists in `test_before.log` and `test_after.log` using the
same backend subset command recorded in `todo.md`:

`cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log"`

Both logs show `100% tests passed, 0 tests failed out of 180`, and include
`backend_aarch64_instruction_dispatch`. This is sufficient for the Route 3
AArch64 backend boundary slice and the docs/lifecycle-only planning slice.
A full acceptance pass was not run in this window; I do not see a scope reason
to require it before the next plan activation.

## Judgments

- Idea-alignment judgment: `matches source idea`
- Runbook-transcription judgment: `plan matches idea`
- Route-alignment judgment: `on track`
- Technical-debt judgment: `acceptable`
- Validation sufficiency: `narrow proof sufficient`
- Reviewer recommendation: `continue current route`
