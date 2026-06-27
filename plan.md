# Prepared Contract Verifier And Owner Taxonomy Runbook

Status: Active
Source Idea: ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md

## Purpose

Install an early prepared-contract verifier/report vocabulary so malformed or
missing prepared facts are classified before MIR RV64/AArch64 consumers try to
lower them.

## Goal

Create the initial verifier surface, prove at least three fact-family owner
classifications, and publish the taxonomy matrix for follow-up ideas 414-418.

## Core Rule

Prepared producer defects must fail before target consumers recover or infer
the missing fact. Do not move ABI homes, aggregate layout, helper operand
addresses, global initializer bytes, or BIR constant semantics into target
fallback code.

## Read First

- `ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md`
- `ideas/open/414_typed_prepared_call_argument_contracts.md`
- `ideas/open/415_prepared_value_materialization_contracts.md`
- `ideas/open/416_prepared_helper_operand_home_contracts.md`
- `ideas/open/417_prepared_storage_layout_and_initializer_contracts.md`
- `ideas/open/418_prepared_target_consumer_boundary_audit.md`
- `docs/prepared_fact_contracts/README.md`
- prepared publication and test surfaces under `src/backend/prealloc/`,
  `src/backend/bir/lir_to_bir/`, `src/backend/mir/riscv/codegen/`,
  `src/backend/mir/aarch64/codegen/`, and `tests/backend/`

## Current Targets

- Prepared fact families: value homes, call argument plans, memory accesses,
  helper operand homes, and publication facts.
- Initial owner classes: coherent, producer-missing, producer-incoherent,
  target-unsupported-but-coherent, and pre-prepared semantic failure.
- Durable handoff artifact:
  `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`

## Non-Goals

- Do not rewrite all prepared structs in this runbook.
- Do not implement broad RV64 object lowering.
- Do not change gcc_torture allowlists, runtime comparison, unsupported
  markers, or test contracts to claim progress.
- Do not use RV64 gcc_torture pass count as the close gate.
- Do not add testcase-shaped shortcuts for named torture files.

## Working Model

The verifier/report pass should run after prepared publication and before
target prepared consumers. The first implementation can be incremental, but it
must prevent selected malformed facts from reaching consumers unchanged and
must expose diagnostics that route ownership to prepared producers, target
lowering, or pre-prepared semantics.

## Execution Rules

- Keep each code step behavior-preserving except for precise fail-closed
  diagnostics on malformed selected fact families.
- Prefer existing missing-fact/status vocabulary when it already identifies the
  owner clearly.
- Add focused prepared/backend tests for each selected owner category.
- If an issue belongs to a different durable initiative, record it in
  `todo.md` and ask the supervisor to route lifecycle state instead of silently
  expanding this plan.
- For code-changing steps, prove `cmake --build build -j` and the delegated
  focused test subset. Treat broad CTest as the acceptance checkpoint when the
  supervisor asks for close or milestone validation.

## Steps

### Step 1. Inventory Existing Prepared Fact Failures

Goal: identify the concrete status enums, missing-fact paths, and target
rejection paths that the first verifier slice will normalize.

Primary targets:

- `src/backend/prealloc/`
- `src/backend/bir/lir_to_bir/`
- `src/backend/mir/riscv/codegen/`
- `src/backend/mir/aarch64/codegen/`
- prepared/backend tests under `tests/backend/`

Actions:

- Inspect current diagnostics for value homes, call argument plans, memory
  accesses, helper operand homes, and publication facts.
- Choose at least three fact families for the first verifier/report slice.
- Classify each selected family against the owner classes in this runbook.
- Record file references and candidate diagnostics in `todo.md` before code
  changes start.

Completion check:

- `todo.md` names the selected fact families, their current failure surfaces,
  and the intended owner class for each initial verifier diagnostic.

### Step 2. Add The Initial Verifier Report Surface

Goal: create the shared data model and invocation point for prepared contract
classification before target consumers run.

Actions:

- Add a verifier/report API that can represent the selected owner classes and
  fact-family identifiers without target-local vocabulary.
- Wire the verifier after prepared publication and before RV64/AArch64 prepared
  consumers for the selected route.
- Keep existing producer diagnostics where they are more precise; adapt them
  into the verifier report instead of replacing them with generic errors.
- Make malformed selected facts fail closed before target lowering.

Completion check:

- The build succeeds, and at least one selected malformed fact is stopped by
  the verifier/report path before target consumer lowering.

### Step 3. Implement Three Fact-Family Classifications

Goal: prove the taxonomy across at least three selected prepared fact families.

Actions:

- Add verifier checks for three families selected in Step 1.
- Cover at least one producer-missing or producer-incoherent case and one
  target-unsupported-but-coherent case if the selected families expose both.
- Add focused tests that assert owner-classified diagnostics rather than only
  target rejection strings.
- Do not add target fallback inference for missing producer facts.

Completion check:

- Focused tests prove the three selected families emit owner-classified
  diagnostics, and no selected malformed prepared fact reaches the target
  consumer unchanged.

### Step 4. Publish The Taxonomy Matrix

Goal: create the handoff artifact consumed by ideas 418 and 414-417.

Primary target:

- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`

Actions:

- Document the verifier categories and their ownership meaning.
- Add fact-family rows for the selected verifier families and placeholder row
  IDs for remaining in-scope families.
- Record owner decisions, example diagnostics, source file references, and
  follow-up row IDs that downstream ideas must cite.
- Explicitly distinguish target-unsupported coherent facts from
  producer-missing or producer-incoherent facts.

Completion check:

- The matrix is specific enough for idea 418 to audit target consumers and for
  ideas 414-417 to cite consumed taxonomy rows.

### Step 5. Acceptance Validation And Close Readiness

Goal: prove the active runbook satisfies idea 413 without claiming downstream
contract migrations are complete.

Actions:

- Run the delegated focused proof commands for the final implementation slice.
- Run or request supervisor-run default CTest:
  `ctest --test-dir build -j --output-on-failure`.
- Review the diff for testcase overfit, expectation weakening, unsupported
  downgrades, or target-side recovery of producer facts.
- Leave downstream work in ideas 418 and 414-417; do not absorb it into this
  runbook.

Completion check:

- The verifier/report surface runs before selected target consumers, at least
  three fact families produce owner-classified diagnostics, the taxonomy matrix
  exists, and default CTest has no accepted regression.
