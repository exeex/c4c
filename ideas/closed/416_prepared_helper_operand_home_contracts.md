# Prepared Helper Operand Home Contracts

Status: Closed
Type: Follow-up contract refactor idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Depends On: initial taxonomy from `ideas/closed/413_prepared_contract_verifier_and_owner_taxonomy.md` and target-consumer findings from `ideas/closed/418_prepared_target_consumer_boundary_audit.md`
Handoff Inputs: `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`, `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`

## Goal

Normalize helper and intrinsic operand homes into typed prepared contracts,
starting with variadic helpers.

## Why This Exists

Idea 408 showed that `va_start` needs an explicit destination `va_list` address
fact. `PreparedVariadicEntryHelperOperandHomes` currently stores multiple
helper-specific optional homes and access plans in one struct. Missing facts are
assembled as strings per helper, which makes ownership visible but not yet a
typed contract.

## In Scope

- Split variadic helper operand contracts into typed per-helper payloads.
- Use the early target-consumer audit to find helper lowering paths that infer
  operand homes from stack layout, BIR shape, or source spelling.
- Cite the consumed taxonomy and target-consumer audit rows in
  `docs/prepared_fact_contracts/helper_operand_home_contract_plan.md`.
- Add completeness checks for `va_start`, scalar `va_arg`, aggregate `va_arg`,
  and `va_copy`.
- Preserve precise producer diagnostics for missing helper operands.
- Migrate RV64 variadic helper lowering to consume typed helper contracts.
- Add focused variadic prepared/backend tests.

## Out Of Scope

- Rewriting variadic ABI policy wholesale.
- Inferring helper operand addresses in RV64 from stack layout, BIR shape, or
  source spelling.
- Broad intrinsic refactors beyond the helper contract pattern selected here.

## Acceptance Criteria

- Migrated helper operands cannot be represented as mismatched optional fields.
- The helper-operand contract plan names the idea 413/418 rows consumed by this
  slice, or explicitly records that no applicable target-consumer audit row
  exists for the selected helper.
- Missing helper operand homes fail in prepared verification or producer
  diagnostics before target lowering.
- Default `ctest --test-dir build -j --output-on-failure` / normal CTest does
  not regress.
- RV64 gcc_torture regressions are acceptable only as precise fail-closed
  helper contract diagnostics.

## Completion Notes

Closed after the active runbook completed Step 8. Variadic helper operands for
`va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy` now use typed
prepared payloads through producer verification and AArch64/RV64 target
consumers, with legacy optional fields retained only as documented
producer/fixture compatibility storage. The helper contract plan cites the
consumed idea 413/418 taxonomy and audit rows. Focused helper validation passed
11/11, normal CTest passed 3356/3356, and the close-time regression guard
accepted `test_baseline.log` to `test_after.log` with no new failures.

## Reviewer Reject Signals

- Reject RV64 object-emitter code that synthesizes `va_list` addresses or other
  helper homes when typed helper facts are absent.
- Reject merely renaming missing fact strings while the optional-bag contract
  remains the active consumer API for the migrated helper.
- Reject broad variadic rewrites that do not prove helper operand ownership.
- Reject named-case-only fixes for `va-arg-21.c` or related torture files.
- Reject expectation rewrites, allowlist filtering, or weaker runtime checks.
