# Prepared Aggregate `va_arg` Access Plan Runbook

Status: Active
Source Idea: ideas/open/246_prepared_aggregate_va_arg_access_plan.md
Activated from: ideas/open/246_prepared_aggregate_va_arg_access_plan.md
Split from parked idea: ideas/open/243_aarch64_variadic_machine_node_consumption.md

## Purpose

Provide the prepared/shared aggregate `va_arg` access-plan authority that
AArch64 selected machine-node lowering needs before it can consume aggregate
variadic helper effects without target-local ABI reconstruction.

## Goal

Expose `helper_operand_homes.va_arg_aggregate.aggregate_access_plan` for a
representative supported aggregate `va_arg` path, prove it through prepared
facts and printer coverage, and keep selected AArch64 aggregate machine-node
consumption deferred to idea 243.

## Core Rule

Shared preparation may compute and carry aggregate variadic access facts;
AArch64 target lowering must only consume those facts and must not reconstruct
aggregate classification, register-save coordinates, overflow progression,
copy extent, `va_list` layout, named argument counts, or scratch policy.

## Read First

- `ideas/open/246_prepared_aggregate_va_arg_access_plan.md`
- `ideas/open/243_aarch64_variadic_machine_node_consumption.md`
- `ideas/closed/244_aarch64_variadic_prepared_storage_and_helper_authority.md`
- `ideas/closed/245_prepared_scalar_va_arg_access_plan.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- focused prepared and variadic backend tests under `tests/backend/bir/`
- current fail-closed aggregate selected-node tests under `tests/backend/mir/`

## Current Targets

- `PreparedVariadicHelperOperandHomes` aggregate `va_arg` carrier.
- New prepared/shared aggregate access-plan record and complete-fact checks.
- AAPCS64 preparation path that already owns variadic helper classification,
  register-save storage, overflow storage, aggregate homes, and helper
  resources.
- Prepared printer output for the aggregate access-plan fields.
- Focused tests that prove the fact exists independently from selected
  machine-node consumption.

## Non-Goals

- Do not implement selected AArch64 machine-node consumption for aggregate
  `va_arg`; that resumes in idea 243 after this prerequisite closes.
- Do not infer AAPCS64 aggregate classification, source coordinates,
  size/alignment, copy extent, overflow progression, or `va_list` layout inside
  AArch64 target lowering.
- Do not weaken fail-closed diagnostics or mark aggregate `va_arg` selected
  nodes as supported before the active prerequisite closes.
- Do not broaden into scalar `va_arg`, `va_start`, `va_copy`, unrelated
  AArch64 machine-node families, or frame-allocation policy.

## Working Model

Idea 243 Step 4 stopped with the precise missing prepared/shared fact
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`. Existing
prepared facts already expose the aggregate destination payload home and source
`va_list` home, but do not expose the semantic source selection, payload copy
extent, aggregate size/alignment, or progression facts needed by selected
machine-node consumers.

This runbook should add that carrier in shared preparation, prove it with
prepared/printer tests, and leave AArch64 aggregate selected-node consumption
fail-closed until idea 243 is reactivated.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Add focused prepared-carrier tests before changing printer expectations.
- Preserve fail-closed behavior for missing or incomplete aggregate
  access-plan authority.
- Treat fixture-name matching, expectation-only changes, unsupported
  downgrades, diagnostic-only rewrites, or target-local ABI reconstruction as
  route drift.
- For every code-changing packet, prove with a build plus the
  supervisor-chosen focused prepared/variadic backend subset. Escalate to
  broader backend validation after shared prepared-carrier or printer behavior
  changes.

## Ordered Steps

### Step 1: Define Prepared Aggregate Access Plan Carrier

Goal: Add the typed prepared/shared record and completeness checks for
aggregate `va_arg` access planning.

Actions:

- Extend the variadic helper operand-home carrier with an optional aggregate
  access-plan record for aggregate `va_arg` helpers.
- Include source class, payload size/alignment, source storage coordinates,
  destination payload relationship, copy extent, and `va_list` progression
  facts needed by later selected-node lowering.
- Keep missing or incomplete aggregate access-plan facts observable through the
  existing fail-closed missing-fact path.

Completion check:

- The prepared model can represent complete and missing aggregate access-plan
  authority without changing selected AArch64 aggregate machine-node support.

### Step 2: Populate Aggregate Access Plans In Shared Preparation

Goal: Fill the aggregate access-plan carrier from the AAPCS64 prepared helper
facts that already own variadic ABI decisions.

Actions:

- Populate the aggregate access plan for representative supported aggregate
  `va_arg` helper records.
- Derive source selection, payload coordinates, copy extent, and progression
  only in shared preparation, not in AArch64 target lowering.
- Preserve explicit failure for unsupported or incomplete aggregate facts.

Completion check:

- Focused preparation coverage sees populated
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan` for a supported
  aggregate path and explicit missing-fact diagnostics when it cannot be built.

### Step 3: Print And Prove Prepared Aggregate Access Plans

Goal: Make the new prepared fact inspectable and covered without claiming
selected machine-node consumption.

Actions:

- Extend prepared printer output for aggregate access-plan fields.
- Add focused printer/preparation assertions for source class, size/alignment,
  source coordinates, destination payload, copy extent, and progression.
- Ensure existing AArch64 aggregate selected-node tests still fail closed until
  idea 243 consumes the fact.

Completion check:

- Focused prepared/printer tests prove the new aggregate access-plan carrier,
  and selected AArch64 aggregate `va_arg` consumption remains out of scope.

### Step 4: Validate And Hand Back To Idea 243

Goal: Prove the prerequisite and preserve the lifecycle handoff point.

Actions:

- Run the supervisor-chosen build and focused prepared/variadic backend subset.
- Escalate to broader backend validation if shared prepared-carrier changes
  affect more than the aggregate helper family.
- Summarize the fact supplied and any remaining unsupported aggregate cases in
  `todo.md`.
- Ask the supervisor to close this prerequisite and reactivate idea 243 at
  Step 4 when acceptance criteria are met.

Completion check:

- The prerequisite has fresh proof for the aggregate access-plan fact and idea
  243 can resume consuming
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan` as shared
  prepared authority.
