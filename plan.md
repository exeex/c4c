# Prepared Helper Operand Home Contracts Runbook

Status: Active
Source Idea: ideas/open/416_prepared_helper_operand_home_contracts.md

## Purpose

Normalize helper and intrinsic operand homes into typed prepared contracts,
starting with variadic helpers and preserving the producer-owned diagnostic
boundary.

## Goal

Replace the variadic helper optional-bag API with typed per-helper contracts
that RV64 and AArch64 helper lowering can consume without target-side operand
home inference.

## Core Rule

Do not synthesize `va_list` addresses, scalar or aggregate `va_arg` homes,
`va_copy` homes, inline-asm homes, or helper address selections in target
lowering when the prepared fact is missing or incoherent. Missing or
contradictory helper facts must fail closed as prepared producer defects.

## Read First

- `ideas/open/416_prepared_helper_operand_home_contracts.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`
- `src/backend/prealloc/prepared_contract_verifier.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `src/backend/bir/prepare/*`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- RV64 variadic/helper lowering surfaces under `src/backend/mir/riscv/codegen/`

## Current Targets

- Taxonomy rows:
  - `TAX-FAM-VARIADIC-HELPER-OPERAND-HOMES-001`
  - `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001`
  - helper-carrier portions of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`
  - helper address-selection portions of `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`
- Idea 418 audit rows:
  - `418-AUD-RV64-INLINEASM-COHERENT-001`
  - `418-AUD-A64-VARIADIC-COHERENT-001`
  - `418-AUD-A64-INLINEASM-COHERENT-001`
- Required no-row note:
  - idea 418 found no selected recovery row requiring immediate idea 416
    cleanup; use the coherent rows as boundaries and add fail-closed checks if
    implementation reveals missing helper-carrier fields.
- Initial migrated helpers:
  - `va_start`
  - scalar `va_arg`
  - aggregate `va_arg`
  - `va_copy`

## Non-Goals

- Do not rewrite variadic ABI policy wholesale.
- Do not infer helper operand addresses from stack layout, BIR shape, or source
  spelling.
- Do not perform broad intrinsic refactors beyond the selected helper-contract
  pattern.
- Do not claim progress through expectation rewrites, allowlist filtering,
  unsupported downgrades, or named torture-case fixes.

## Working Model

- `PreparedVariadicEntryHelperOperandHomes` is the compatibility surface to
  retire or narrow. The replacement should expose typed per-helper payloads, so
  a migrated helper cannot be represented by mismatched optional fields from
  another helper.
- Completeness checks should distinguish:
  - absent required helper facts as `producer_missing`
  - present but wrong-helper or incomplete helper payloads as
    `producer_incoherent`
  - coherent but unsupported helper payload forms as
    `target_unsupported_but_coherent`
- Producer diagnostics must remain precise enough to name the missing helper
  operand home or access-plan field before target lowering proceeds.

## Execution Rules

- Keep changes behavior-preserving except for earlier fail-closed diagnostics
  on missing or incoherent helper facts.
- Prefer narrow typed payloads over adding more optional fields to the existing
  helper bag.
- Migrate one helper family at a time when possible, with focused tests for the
  migrated contract shape and diagnostics.
- Cite the consumed taxonomy and audit rows in
  `docs/prepared_fact_contracts/helper_operand_home_contract_plan.md` before
  relying on the route as implementation authority.
- For code-changing steps, run a fresh build or compile proof plus the narrow
  prepared/backend tests selected by the supervisor.
- Before treating the idea as complete, run normal CTest:
  `ctest --test-dir build -j --output-on-failure`.

## Ordered Steps

### Step 1: Document the Helper Contract Route

Goal: Create the durable helper-operand contract plan that ties this runbook to
the idea 413 taxonomy and idea 418 audit handoff.

Primary target:

- `docs/prepared_fact_contracts/helper_operand_home_contract_plan.md`

Actions:

- Cite `TAX-FAM-VARIADIC-HELPER-OPERAND-HOMES-001` and
  `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001`.
- Cite `418-AUD-RV64-INLINEASM-COHERENT-001`,
  `418-AUD-A64-VARIADIC-COHERENT-001`, and
  `418-AUD-A64-INLINEASM-COHERENT-001`.
- Record the explicit no-applicable-recovery-row note from idea 418 for idea
  416.
- Name the selected first helper family and define the missing/incoherent
  owner decisions for `va_start`, scalar `va_arg`, aggregate `va_arg`, and
  `va_copy`.

Completion check:

- The plan document exists, contains row IDs from both handoff docs, and
  describes how target helper lowering must fail closed rather than infer homes.

### Step 2: Inventory Current Variadic Helper Payloads

Goal: Identify the current optional-bag fields, producers, verifier checks, and
target consumers before changing contracts.

Primary targets:

- `src/backend/bir/prepare/*`
- `src/backend/prealloc/prepared_contract_verifier.*`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- RV64 variadic/helper lowering surfaces under `src/backend/mir/riscv/codegen/`
- focused variadic prepared/backend tests

Actions:

- Trace where `PreparedVariadicEntryHelperOperandHomes` is populated,
  verified, and consumed.
- Separate `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy`
  fields into the payload each helper actually needs.
- Record any existing diagnostics that already name helper-specific missing
  facts.
- Identify the narrow test commands that cover the current producer,
  verifier, and target-consumer behavior.

Completion check:

- `todo.md` records the concrete owned files, narrow proof command, and first
  helper packet boundary for implementation.

### Step 3: Introduce Typed Per-Helper Payloads

Goal: Add typed helper payload structures without changing target behavior yet.

Primary targets:

- prepared variadic entry/helper data structures
- helper lookup/accessor APIs
- existing prepared variadic tests

Actions:

- Define typed payloads for `va_start`, scalar `va_arg`, aggregate `va_arg`,
  and `va_copy`.
- Add conversion or compatibility accessors only where needed to keep the first
  implementation slice small.
- Prevent mismatched helper payloads from being represented as coherent facts.
- Keep old consumers compiling until their migration step.

Completion check:

- Build proof passes, and focused prepared tests show helper payloads can be
  produced and queried without mismatched optional-field combinations.

### Step 4: Move Completeness Verification to Typed Payloads

Goal: Verify helper-specific payload completeness with producer-owned owner
classification before target lowering.

Primary targets:

- `src/backend/prealloc/prepared_contract_verifier.*`
- prepared contract classification tests

Actions:

- Verify `va_start` destination storage and destination address facts.
- Verify scalar `va_arg` source `va_list`, result home, scalar payload ABI, and
  scalar access plan.
- Verify aggregate `va_arg` source `va_list`, aggregate destination payload,
  aggregate payload ABI, and aggregate access plan.
- Verify `va_copy` destination and source `va_list` homes.
- Preserve precise missing-fact detail strings while routing them through typed
  payload checks.

Completion check:

- Missing helper facts classify as `producer_missing`; wrong-helper or
  incomplete present payloads classify as `producer_incoherent`; focused
  verifier tests cover each migrated helper.

### Step 5: Migrate AArch64 Helper Lowering

Goal: Consume typed variadic helper contracts in AArch64 lowering without
target-side home inference.

Primary targets:

- `src/backend/mir/aarch64/codegen/variadic.cpp`
- AArch64 helper diagnostic/report tests

Actions:

- Replace optional-bag reads with typed helper payload accessors.
- Attach existing prepared contract reports at fail-closed rejection points.
- Keep coherent helper lowering behavior unchanged for supported payloads.
- Add or adjust tests for `va_start`, scalar `va_arg`, aggregate `va_arg`, and
  `va_copy` contract failures.

Completion check:

- AArch64 focused tests pass and diagnostics remain producer-owned for missing
  or incoherent helper facts.

### Step 6: Migrate RV64 Helper Lowering

Goal: Migrate RV64 variadic helper lowering to consume typed helper contracts
and reject missing facts before target recovery.

Primary targets:

- RV64 variadic/helper lowering surfaces under `src/backend/mir/riscv/codegen/`
- RV64 prepared/backend variadic tests

Actions:

- Replace helper optional-bag access with typed payload accessors.
- Ensure RV64 does not synthesize `va_list` addresses or helper homes from
  stack layout, BIR shape, or source spelling.
- Add focused RV64 tests for coherent helper lowering and fail-closed missing
  helper contracts.

Completion check:

- RV64 focused tests pass. Any RV64 gcc_torture regression is limited to a
  precise fail-closed helper contract diagnostic, not weaker expectations.

### Step 7: Retire the Optional-Bag Consumer API

Goal: Remove or quarantine the old mismatched optional-field API for migrated
helpers.

Primary targets:

- prepared helper data structures and accessors
- verifier and target consumers touched by earlier steps

Actions:

- Remove stale optional fields or make them private compatibility storage with
  typed access only.
- Update tests so migrated helpers can no longer pass by mixing fields from
  different helper kinds.
- Re-scan for target consumers that still read helper homes through the old
  optional-bag API.

Completion check:

- No migrated target consumer depends on the old optional-bag contract, and
  tests reject mismatched helper payload combinations.

### Step 8: Final Validation and Closure Readiness

Goal: Prove the helper contract migration is acceptance-ready under the source
idea criteria.

Actions:

- Run the supervisor-selected focused variadic prepared/backend tests.
- Run normal CTest:
  `ctest --test-dir build -j --output-on-failure`.
- Review diffs for reject signals:
  - target-side helper home synthesis
  - string-only missing-fact renames with the old optional bag still active
  - broad variadic ABI rewrites
  - named-case-only fixes
  - expectation rewrites or weaker runtime checks

Completion check:

- Focused proof and normal CTest pass, or any remaining RV64 gcc_torture
  failures are precise fail-closed helper contract diagnostics accepted by the
  supervisor.
