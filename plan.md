# Return-Chain Import and Naming Clarification Runbook

Status: Active
Source Idea: ideas/open/197_return_chain_import_and_naming_clarification.md

## Purpose

Import the completed return-chain owner/schema result as a distinct route line
that future `PreparedBirModule` retirement and Phase E analysis can cite
without folding it into unrelated route families.

## Goal

Produce a durable return-chain import note that names the accepted source
materials, defines the stable route/owner-schema line name, and separates
target-neutral return-chain facts from AArch64 or prepared-owned policy.

## Core Rule

Keep return-chain as its own owner/schema line. Do not merge it into Route 1
producer identity, Route 7 comparison provenance, predecessor rescans, name
matching, or a generic route-index facade.

## Read First

- `ideas/open/197_return_chain_import_and_naming_clarification.md`
- `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`
- `ideas/closed/176_return_chain_owner_schema_decision.md`
- `ideas/closed/177_bir_return_chain_schema_index.md`
- `ideas/closed/178_bir_return_chain_oracle_equivalence.md`
- `ideas/closed/179_bir_return_chain_consumer_migration.md`
- `ideas/closed/180_bir_return_chain_prepared_api_contraction.md`
- Current readiness docs under `docs/bir_prealloc_fusion/` that cite
  return-chain, `PreparedFunctionLookups`, or `PreparedBirModule`.

## Scope

- Durable docs note for future retirement analysis.
- Stable naming for the return-chain route or owner/schema line.
- Accepted target-neutral return-chain identity facts.
- AArch64 and prepared-owned policy boundaries.
- Residual prepared return-chain oracle/fallback helper status, including
  explicit absence when the public helper surface has already been contracted.

## Non-Goals

- Do not reopen or reimplement closed return-chain ideas 176-180.
- Do not change implementation code, tests, expectations, unsupported markers,
  or AArch64 return ABI/emission behavior.
- Do not delete prepared return-chain helpers as part of this docs packet.
- Do not open draft 155 or claim broad `PreparedBirModule` retirement
  readiness.

## Working Model

The return-chain line owns target-neutral BIR identity: function/block context,
same-block scalar binary chain value, terminal return value, and optional
immediate next-operand value. Target lowering owns homes, registers, ABI return
placement, alias checks, scratch choice, final ALU records, and emission order.

## Execution Rules

- Treat this as docs/lifecycle-only unless the supervisor explicitly delegates
  implementation work.
- Preserve historical closed idea files.
- Cite closed idea numbers and the decision doc directly so future plans do not
  infer return-chain status from route family names.
- If searches discover a separate implementation cleanup, record it in
  `todo.md` as a follow-up note for supervisor routing instead of expanding
  this plan.

## Steps

### Step 1: Inventory Return-Chain Source Materials

Goal: collect the exact closed artifacts and docs that future plans should cite.

Primary targets:
- `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`
- `ideas/closed/176_return_chain_owner_schema_decision.md`
- `ideas/closed/177_bir_return_chain_schema_index.md`
- `ideas/closed/178_bir_return_chain_oracle_equivalence.md`
- `ideas/closed/179_bir_return_chain_consumer_migration.md`
- `ideas/closed/180_bir_return_chain_prepared_api_contraction.md`

Actions:
- Inspect the decision, schema/index, oracle-equivalence, consumer-migration,
  and API-contraction completion facts.
- Identify the stable source list the durable import note must name.
- Check current docs references to return-chain, `PreparedFunctionLookups`, and
  `PreparedBirModule` for citation needs.

Completion check:
- `todo.md` names the closed artifacts, current docs references, and any
  citation hazards found.

### Step 2: Classify Neutral Facts and Target Policy

Goal: separate accepted return-chain identity facts from AArch64 or
prepared-owned policy.

Actions:
- Extract the accepted target-neutral facts from the decision doc and closed
  implementation notes.
- List AArch64 policy that must stay target-local: ABI return moves, value
  homes, register conversion/parsing, alias checks, scratch selection, final
  ALU records, and emission order.
- List prepared-owned surfaces that are historical, diagnostic, fallback, or
  absent after API contraction.

Completion check:
- `todo.md` contains a classification table or equivalent summary with no
  target policy imported into the route identity.

### Step 3: Define Naming and Future Citation Rules

Goal: define how future plans should name and cite the return-chain line.

Actions:
- Choose a durable name for the line, such as `Return-chain owner/schema line`
  or an equivalent explicit form.
- State how the line should appear in `PreparedFunctionLookups` and
  `PreparedBirModule` readiness analysis.
- State negative citation rules: do not cite it as Route 1, Route 7,
  predecessor rescan, name matching, or generic route-index progress.

Completion check:
- `todo.md` records the accepted name and citation rules.

### Step 4: Write the Durable Import Note

Goal: create the docs artifact future Phase E work can cite directly.

Primary target:
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`

Actions:
- Write a concise note naming the source documents and closed ideas.
- Include separate sections for target-neutral facts, target-local policy,
  prepared helper/oracle status, and future citation rules.
- Explain how return-chain should appear in `PreparedFunctionLookups` and
  `PreparedBirModule` readiness analysis without merging into Routes 1 or 7.

Completion check:
- The new docs artifact satisfies the source idea acceptance criteria.

### Step 5: Final Consistency Check

Goal: verify the docs packet resolves the import ambiguity without claiming
implementation progress.

Actions:
- Check the artifact against the source idea acceptance criteria and reviewer
  reject signals.
- Confirm the diff contains only lifecycle/docs changes.
- Run formatting or whitespace checks appropriate for changed Markdown files.

Completion check:
- `todo.md` records the final proof and states whether the source idea is ready
  for plan-owner close.
