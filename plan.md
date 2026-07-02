# RV64 Move-Bundle Materialization Runbook

Status: Active
Source Idea: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md

## Purpose

Turn the 151 current `coherent_rv64_mir_materialization` rows from the
move-bundle target-shape classification into small, semantic RV64/MIR lowering
packets.

## Goal

Implement general RV64 move-bundle materialization for rows whose prepared
source and destination homes are already coherent, without consuming prepared
authority gaps or evidence-gap rows.

## Core Rule

Lower from prepared move facts, value homes, scalar type facts, and move shape.
Do not infer missing homes from testcase names, raw target spellings, final
register text, or expected assembly.

## Read First

- `ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md`
- `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
- `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md`
- Current per-case logs referenced by the classification table

## Current Scope

- Lane filter: `first_owner_lane=coherent_rv64_mir_materialization`
- Current row count: 151
- Primary subqueue: 130
  `consumer_register_to_stack/register_to_stack_slot` rows
- Secondary subqueues:
  - 15 `consumer_register_to_stack/rematerializable_immediate_to_stack_slot`
  - 3 `phi_join_register_to_register/rematerializable_immediate_to_register`
  - 2 `consumer_stack_to_stack/stack_slot_to_stack_slot`
  - 1 `phi_join_register_to_register/select_publication_immediate_to_register`

## Non-Goals

- Do not repair the 31 prepared/module target-shape authority rows here.
- Do not classify or implement the `src/960209-1.c` evidence-gap row here.
- Do not change BIR semantic producer admission or prepared authority unless a
  row proves it was misrouted; route that row to the proper source idea instead.
- Do not touch F128 policy, helper ABI, quarantine, expectation files,
  unsupported markers, allowlists, or runtime comparison behavior.
- Do not special-case individual filenames.

## Working Model

- Treat the classification table as the queue definition, not as permission to
  implement all 151 rows in one patch.
- Start with one coherent move-shape family and prove it with representative
  rows before expanding coverage.
- Any row that lacks coherent prepared source or destination facts leaves this
  implementation lane and belongs in a producer or evidence idea.
- Prefer narrow code packets that improve an observed semantic move family and
  leave `todo.md` with exact proof commands and residual row notes.

## Execution Rules

- Keep routine execution progress in `todo.md`.
- Keep implementation packets small enough for focused build and gcc_torture
  subset proof.
- Use the supervisor-selected proof command for each executor packet.
- If a packet changes shared RV64 materialization helpers, include a build
  proof and a focused gcc_torture subset; escalate to broader validation when
  multiple subqueues or shared storage semantics are affected.
- Reject any path that makes the known rows pass by expectation rewrites,
  unsupported downgrades, allowlist filtering, or diagnostic-only changes.

## Steps

### Step 1: Reconstruct The First Materialization Packet

Goal: select a coherent first implementation packet from the 151-row lane.

Primary target: the largest ready family,
`consumer_register_to_stack/register_to_stack_slot`, unless current evidence
shows a smaller family is the safer first semantic unit.

Actions:

- Filter the classification TSV to
  `first_owner_lane=coherent_rv64_mir_materialization`.
- Recount the five subqueues and record representative rows for the chosen
  first packet in `todo.md`.
- Inspect current case logs and prepared output for those representatives to
  identify the prepared facts RV64 must consume.
- Name the exact RV64/MIR implementation surface to inspect next.
- Exclude prepared-authority and evidence-gap rows explicitly.

Completion check:

- `todo.md` records the chosen first packet, representative rows, expected
  proof subset, and any rows rerouted out of this idea.
- No implementation files are changed in this step unless the executor packet
  explicitly owns the first code change.

### Step 2: Implement Register-To-Stack Move Materialization

Goal: make RV64 consume coherent register-to-stack prepared move facts without
guessing homes.

Primary target:
`consumer_register_to_stack/register_to_stack_slot` rows.

Actions:

- Inspect existing RV64 move-bundle, object lowering, storage, stack-slot, and
  immediate materialization helpers before editing.
- Add or extend the semantic lowering path for source register home to
  destination stack-slot home.
- Preserve scalar width, sign/zero-extension, and stack-slot offset authority
  from prepared facts.
- Keep the rule independent of filename, testcase shape, and expected assembly
  spelling.
- Route any missing prepared facts to the prepared authority idea instead of
  inferring them in RV64.

Completion check:

- A focused representative subset no longer fails with the generic
  `unsupported_move_bundle_target_shape` materialization failure for this
  family.
- Build proof and the delegated gcc_torture proof command are recorded in
  `test_after.log` and summarized in `todo.md`.

### Step 3: Add Rematerializable Immediate Move Coverage

Goal: materialize coherent immediate sources for the ready stack/register
destination families.

Primary targets:

- `consumer_register_to_stack/rematerializable_immediate_to_stack_slot`
- `phi_join_register_to_register/rematerializable_immediate_to_register`

Actions:

- Reuse existing immediate materialization helpers where possible.
- Verify the prepared source is genuinely rematerializable and carries enough
  scalar type information.
- Handle register and stack destinations through semantic homes, not through
  row-specific constants.
- Keep F128 and prepared-authority gaps outside this route.

Completion check:

- Representative immediate-source rows advance for the semantic reason
  described above.
- Narrow proof covers both stack and register destination variants when both
  are touched.

### Step 4: Add Stack-To-Stack Move Coverage

Goal: support coherent stack-slot to stack-slot prepared moves without
fabricating address facts.

Primary target:
`consumer_stack_to_stack/stack_slot_to_stack_slot` rows.

Actions:

- Confirm both source and destination stack slots have prepared authority for
  offset, size, and alignment.
- Materialize through safe load/store or temporary-register paths already used
  by RV64 object lowering.
- Reject rows whose source-home or destination-home authority is inconsistent.

Completion check:

- Stack-to-stack representatives advance with no prepared-authority inference.
- Focused proof and any residual reroutes are recorded in `todo.md`.

### Step 5: Add Select-Publication Register Move Coverage

Goal: cover the remaining coherent phi/select publication register move row
only after the simpler materialization families are stable.

Primary target:
`phi_join_register_to_register/select_publication_immediate_to_register`.

Actions:

- Inspect the prepared select-publication facts and block-entry publication
  facts for the representative row.
- Reuse the register/immediate materialization path only if the publication
  authority is explicit.
- Keep any missing select authority in the prepared-authority idea.

Completion check:

- The select-publication representative either advances with explicit
  authority or is rerouted with evidence.
- No broader move-bundle family regresses.

### Step 6: Reconcile The 151-Row Lane

Goal: update row-level accounting after the implemented families land.

Actions:

- Re-run or refresh the selected gcc_torture subset under supervisor guidance.
- Compare remaining failures against the original 151-row classification lane.
- Record rows fixed, rows still failing for the same reason, rows blocked by a
  later failure, and rows rerouted to prepared/BIR/evidence ideas.
- Decide whether this source idea is complete, needs another implementation
  runbook, or should be split.

Completion check:

- `todo.md` contains reconciliation counts and proof links.
- Remaining rows have an explicit owner or next lifecycle recommendation.
