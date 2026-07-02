# Prepared Move-Bundle Authority Repair Runbook

Status: Active
Source Idea: ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md

## Purpose

Repair prepared/module target-shape authority gaps that prevent RV64 from
consuming move-bundle facts safely.

## Goal

Make prepared/module output publish auditable move classification, source and
destination homes, scalar type facts, and ABI metadata for the authority-gap
rows, without shifting missing-fact inference into RV64.

## Core Rule

Prepared authority must be explicit before RV64 lowering consumes a row. Do
not infer source homes, destination homes, move classes, scalar types, or ABI
facts from testcase names, raw BIR text, final register spelling, expected
assembly, or the previous RV64 materialization failure shape.

## Read First

- `ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`
- `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
- `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md`
- `docs/rv64_gcc_torture_post_contract/move_bundle_materialization_residual_20_reconciliation.md`
- `ideas/closed/551_rv64_move_bundle_materialization_from_classified_bucket.md`

## Current Scope

- Original lane filter: `first_owner_lane=prepared_module_target_shape_authority_gap`
- Original authority-gap row count: 31
- RV64 materialization carry-in count: 12
- Current total queue: 43 rows before deduplication/revalidation
- Original missing-fact families:
  - 11 `prepared_destination_home_shape_authority` rows
  - 9 `prepared_move_classification_or_source_home_authority` rows
  - 8 `prepared_move_type_authority` rows
  - 2 `prepared_return_abi_destination_home_authority` rows
  - 1 `prepared_select_publication_source_home_authority` row
- Carry-in families from 551 closure:
  - 6 classifier reroutes through
    `ambiguous_non_parallel_multi_source_stack_destination`
  - 6 generic-fragment residuals whose row evidence points at prepared
    authority rather than RV64 materialization

## Non-Goals

- Do not add RV64 materialization rules for rows whose prepared facts are
  still missing or inconsistent.
- Do not repair BIR semantic producer gaps unless a row proves prepared is only
  mirroring missing semantic input; route that as a separate idea.
- Do not touch F128 policy, runtime comparison, expectation files, unsupported
  markers, external allowlists, or unrelated GCC torture accounting.
- Do not hide the authority gap behind renamed diagnostics.
- Do not special-case individual filenames.

## Working Model

- Treat the 43-row queue as a set of authority families, not one broad patch.
- Reconstruct row-level evidence first, then choose the smallest coherent
  prepared publication family.
- Prepared repairs should publish facts where the prepared/module layer owns
  them; rows needing earlier semantic producer facts must be rerouted instead
  of guessed.
- RV64 proof is a dependent consumer check after prepared authority is fixed,
  not the source of authority.

## Execution Rules

- Keep routine execution progress and row accounting in `todo.md`.
- Keep implementation packets narrow and family-based.
- The supervisor chooses each executor proof command and whether broader
  validation is needed.
- Every code-changing packet needs a fresh build proof plus focused
  gcc_torture evidence for the selected family.
- Reject progress based on expectation rewrites, unsupported downgrades,
  allowlist filtering, diagnostic-only renames, or filename-shaped shortcuts.

## Steps

### Step 1: Reconstruct The 43-Row Prepared Authority Queue

Goal: build a current, auditable queue for this idea before editing prepared
publication code.

Actions:

- Regenerate or filter the 31 original
  `prepared_module_target_shape_authority_gap` rows from the classification
  TSV.
- Add the 12 carry-in rows named by the source idea and the 551 closure
  reconciliation.
- Deduplicate the queue and group rows by earliest missing prepared fact:
  destination home, source home or move classification, scalar type, return
  ABI destination home, select-publication source home, or earlier semantic
  producer evidence.
- For each group, record representative rows, current diagnostics, and the
  prepared/module facts RV64 is waiting for.
- Name the prepared/module implementation surfaces to inspect in the next
  packet.

Completion check:

- `todo.md` records the deduplicated row count, group counts, representative
  rows, proof command, and any rows that must be rerouted out of this idea.
- No implementation files are changed unless the executor packet explicitly
  owns a first code change.

### Step 2: Repair Destination-Home Shape Authority

Goal: publish coherent prepared destination-home facts for rows whose earliest
missing authority is destination stack/register shape.

Primary target:
`prepared_destination_home_shape_authority` and carry-in rows with missing or
ambiguous destination-home publication.

Actions:

- Inspect prepared move-bundle publication, module value-home authority, and
  stack-slot/register home construction before editing.
- Add or repair the semantic publication path that should own destination
  home, offset, size, alignment, and register/stack class facts.
- Reject rows whose destination shape depends on missing BIR semantic producer
  facts; route those rows instead of inferring in prepared.
- Preserve existing supported-path behavior and avoid RV64-specific target
  spelling in prepared logic.

Completion check:

- Focused rows no longer fail because destination-home authority is absent or
  ambiguous.
- Build proof and delegated gcc_torture proof are recorded in `test_after.log`
  and summarized in `todo.md`.

### Step 3: Repair Move Classification And Source-Home Authority

Goal: make prepared publication distinguish source-home and move-class facts
for rows currently blocked by missing classification or source authority.

Primary targets:

- `prepared_move_classification_or_source_home_authority`
- carry-in rows from
  `ambiguous_non_parallel_multi_source_stack_destination`
- generic-fragment carry-ins whose evidence points at missing prepared source
  or destination authority

Actions:

- Inspect how prepared derives move class, source home, publication kind, and
  multi-source stack-destination facts.
- Repair only the semantic publication rule supported by prepared inputs.
- Split rows that prove to require earlier BIR semantic producer authority into
  a separate source idea instead of broadening this plan.
- Keep multi-source and non-parallel cases explicit; do not collapse them into
  a generic RV64 materialization path.

Completion check:

- Representatives gain explicit move classification and source-home facts, or
  are rerouted with row-level evidence.
- Focused proof demonstrates the generic materialization failure is not merely
  renamed or hidden.

### Step 4: Repair Scalar Type, Size, And Alignment Authority

Goal: publish scalar type facts needed to make prepared move bundles
consumable without RV64 guessing width or extension behavior.

Primary target:
`prepared_move_type_authority` rows.

Actions:

- Inspect type, size, and alignment propagation into prepared move-bundle
  facts.
- Repair the earliest prepared/module rule that should publish scalar type
  authority.
- Keep extension, truncation, and stack-slot width decisions grounded in
  explicit facts.
- Route F128 or unsupported scalar policy rows to their owning idea instead of
  handling them here.

Completion check:

- Representative rows expose complete scalar type/size/alignment facts or have
  an explicit non-prepared owner.
- Proof covers the affected type family and no unrelated scalar policy changes.

### Step 5: Repair Return ABI And Select-Publication Authority

Goal: cover the smaller authority families after the larger publication paths
are stable.

Primary targets:

- `prepared_return_abi_destination_home_authority`
- `prepared_select_publication_source_home_authority`

Actions:

- Inspect return-value ABI destination publication and select-publication
  source-home publication.
- Repair only explicit ABI or select authority publication.
- Keep rows with missing earlier semantic producer facts outside this packet.

Completion check:

- Return ABI and select-publication representatives either publish auditable
  prepared authority or are rerouted with durable evidence.
- Narrow proof confirms no regression in previously repaired authority
  families.

### Step 6: Reconcile The Prepared Authority Queue

Goal: decide whether this source idea is complete, needs a new runbook, or must
split remaining rows to earlier producer/evidence initiatives.

Actions:

- Re-run the supervisor-selected prepared/RV64 consumer proof subset.
- Compare current outcomes against the reconstructed 43-row queue.
- Count rows repaired, rows rerouted to BIR/evidence/F128/runtime ideas, rows
  blocked by later diagnostics, and rows still missing prepared authority.
- Confirm any dependent RV64 implementation route can consume repaired facts
  without guessing.

Completion check:

- `todo.md` contains reconciliation counts, proof links, residual owners, and a
  lifecycle recommendation.
- No row remains as an unexplained prepared authority gap without a next owner.
