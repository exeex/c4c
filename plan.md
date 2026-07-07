# Target ABI Contract And Value Consumption Research Runbook

Status: Active
Source Idea: ideas/open/585_target_abi_contract_and_value_consumption_research.md
Activated from: ideas/open/585_target_abi_contract_and_value_consumption_research.md

## Purpose

Produce the required target ABI contract research documents before proposing
implementation or contract rewrites.

## Goal

Answer the six source-idea research questions with concrete code and closed
idea evidence, then summarize the architectural follow-up choices in
`docs/target_abi_contract_research/index.md`.

## Core Rule

This is documentation research only. Do not change implementation files, test
expectations, unsupported markers, runtime behavior, or lifecycle history while
executing this runbook.

## Read First

- Source idea:
  `ideas/open/585_target_abi_contract_and_value_consumption_research.md`
- Documentation output directory:
  `docs/target_abi_contract_research/`
- Target profile and target ABI entry points.
- BIR lowering call ABI metadata surfaces.
- Prepared/prealloc register placement, call plan, move bundle, and
  preservation surfaces.
- RV64 and AArch64 prepared-object/backend consumption paths.
- Relevant closed ideas mentioning target ABI facts, value freshness,
  publication, preservation, rematerialization, or move-bundle authority.

## Current Scope

- Create exactly one `index.md` plus exactly six numbered answer files under
  `docs/target_abi_contract_research/`.
- Trace current code paths from target profile creation through BIR ABI facts,
  prepared/prealloc placement, and RV64/AArch64 backend consumption.
- Compare AArch64 and RV64 requirements where the current contract must serve
  both targets.
- Review relevant `ideas/closed/` files as evidence for remaining tails.
- Separate documentation findings, narrow implementation follow-ups, and
  discussion-required architecture work.

## Non-Goals

- Do not implement contract, backend, BIR, prepared/prealloc, x86, AArch64, or
  RV64 changes.
- Do not rewrite closed ideas or alter lifecycle history.
- Do not change tests, expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not collapse the six answer files into one report.
- Do not split one numbered research question across multiple primary answer
  files.
- Do not treat one target-only workaround as evidence that the shared contract
  is sufficient.

## Working Model

- Each numbered answer file owns one research question and follows that
  question's required answer shape from the source idea.
- `index.md` links to all six answer files, summarizes the overall result, and
  includes the final recommendation table.
- Evidence must come from concrete code surfaces and relevant closed idea
  notes, not desired architecture alone.
- Claims about sufficiency must explicitly account for the AArch64/RV64
  dual-platform contract.

## Execution Rules

- Keep routine progress and proof notes in `todo.md`.
- Cite file/function surfaces precisely enough for a later implementation idea
  to start without repeating broad discovery.
- Prefer read-only research commands and documentation edits.
- If the research uncovers implementation work, classify it in the docs and
  leave creation of any implementation source idea to a later lifecycle task.
- Run documentation proof after doc creation or edits, at minimum checking the
  required file set and answer-file count.
- Escalate to supervisor if a required answer cannot be completed without
  changing implementation behavior or source intent.

## Ordered Steps

### Step 1: Trace Target Information Entry

Goal: Produce
`docs/target_abi_contract_research/01_how_target_information_enters_the_pipeline.md`.

Primary targets:
- Target triple/profile creation and `TargetProfile` ownership.
- BIR lowering context and call ABI metadata production.
- Prepared/prealloc mapping from abstract ABI facts to target register
  placements.
- RV64 and AArch64 consumption of the prepared surface.

Actions:
- Trace the actual code path from target triple/profile creation to BIR
  lowering context.
- Trace where BIR call arg/result ABI facts are produced.
- Trace where prepared/prealloc maps abstract ABI facts to target register
  placements.
- Trace where RV64 and AArch64 backends consume the prepared surface.
- Answer whether the current direction of information flow is intentional and
  healthy.
- Record evidence surfaces and proof commands in `todo.md`.

Completion check:
- The answer file exists, answers only question 1, follows the required answer
  shape, and cites concrete code paths from `TargetProfile` through backend
  consumption.

### Step 2: Assess Contract Field Sufficiency

Goal: Produce
`docs/target_abi_contract_research/02_are_current_contract_fields_sufficient.md`.

Actions:
- Table `CallArgAbiInfo`, `CallResultAbiInfo`,
  `PreparedRegisterPlacement`, `PreparedTargetRegisterIdentity`, move bundles,
  call plans, and preservation plans.
- Classify carried facts as semantic ABI facts, allocation-policy facts, or
  physical target facts.
- Compare AArch64 and RV64 requirements explicitly.
- State `sufficient`, `insufficient`, or `sufficient with named limitations`.
- List exact limitations or missing fields when the answer is not simply
  sufficient.

Completion check:
- The answer file gives an evidence-backed sufficiency classification for the
  shared AArch64/RV64 contract.

### Step 3: Map Target Fact Ownership Splits

Goal: Produce
`docs/target_abi_contract_research/03_where_target_facts_are_split.md`.

Actions:
- List each code surface that owns part of the target ABI/register contract.
- State what each surface owns today.
- Mark each split as healthy, suspicious, or accidental.
- Explain whether any split blocks shared x86/AArch64/RV64 register allocator
  goals.
- Identify the smallest surface that could become a coherent target ABI policy
  if consolidation is recommended.

Completion check:
- The answer file distinguishes healthy boundaries from accidental ownership
  splits and names any consolidation candidate without proposing code changes.

### Step 4: Document Prepared Value Consumption

Goal: Produce
`docs/target_abi_contract_research/04_current_prepared_value_consumption_model.md`.

Actions:
- Describe the current reuse/rematerialize/copy/fail decision order using
  concrete code paths.
- Identify producer kinds currently rematerialized explicitly.
- Identify consumer contexts that choose reuse or preservation fallback.
- Identify fail-closed diagnostics that protect unknown authority.
- State whether the decision model is centralized or distributed.

Completion check:
- The answer file makes the current prepared value consumption order
  inspectable and names the code paths behind each branch.

### Step 5: Evaluate Preservation Freshness Risk

Goal: Produce
`docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md`.

Actions:
- Define freshness in the current prepared/prealloc contract.
- Explain when prior preservation is valid today.
- Explain when producer rematerialization should outrank prior preservation.
- Cite concrete stale-home or missing-producer risk evidence from code or
  closed ideas.
- List the minimum facts needed to make preservation authority explicit.

Completion check:
- The answer file separates valid preservation fallback from stale-home or
  missing-producer risk and names the minimum authority facts required.

### Step 6: Review Closed Idea Tails

Goal: Produce
`docs/target_abi_contract_research/06_closed_idea_tails_and_followup_questions.md`.

Actions:
- Review every relevant `ideas/closed/` file for tails related to target ABI
  contracts, value home freshness, publication, preservation,
  rematerialization, or move-bundle authority.
- Quote or summarize each closure tail used as evidence.
- Classify each tail as target ABI policy, value freshness, publication,
  preservation/rematerialization, move-bundle authority, or unrelated.
- Identify whether each tail is already covered by an open idea.
- List remaining concrete follow-up questions.

Completion check:
- The answer file tables every relevant closed idea reviewed and lists the
  concrete follow-up questions left after the research.

### Step 7: Build The Index And Verify The File Set

Goal: Produce `docs/target_abi_contract_research/index.md` and prove the
required documentation shape.

Actions:
- Link to all six numbered answer files.
- Summarize the overall research result.
- Include a final recommendation table classifying each recommended follow-up
  as documentation, narrow implementation idea, or discussion-required
  architecture work.
- Verify the directory contains exactly `index.md` plus the six required
  numbered Markdown answer files.
- Check that no implementation files, test expectations, unsupported markers,
  runtime behavior, source ideas, or closed idea files changed.

Completion check:
- The required seven-document set exists, the answer-file count matches the
  six research questions exactly, and `todo.md` records the documentation proof
  command and result.
