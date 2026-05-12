# Template Record Owner Structured Identity Runbook

Status: Active
Source Idea: ideas/open/177_template_record_owner_structured_identity.md

## Purpose

Replace rendered `SpecializationKey::canonical` text as semantic authority in one bounded template record owner identity path with structured specialization identity that is already available in parser, sema, HIR, or layout ownership state.

## Goal

Finish one template record owner or layout-owner path where structured specialization facts, owner indexes, or domain keys drive semantic identity while rendered specialization text remains display-only payload.

## Core Rule

Do not repair this by normalizing, reparsing, or comparing canonical rendered template strings differently. The route must separate semantic owner identity from display spelling.

## Read First

- `ideas/open/177_template_record_owner_structured_identity.md`
- `SpecializationKey` and `HirRecordOwnerTemplateIdentity` definitions and call sites
- Template record lookup, aggregate layout ownership, and HIR dump paths that consume template owner identity
- Existing focused HIR/frontend tests for template records, specialization owners, and layout ownership

## Current Scope

- Select one template record owner path where `SpecializationKey::canonical` participates in record or layout identity.
- Thread structured specialization identity, owner indexes, or domain keys already available near that path.
- Make missing or mismatched structured owner data explicit for metadata-rich paths.
- Preserve rendered specialization strings for display, dumps, diagnostics, and compatibility-only surfaces.
- Prove the selected path with build proof plus focused HIR/frontend CTest coverage.

## Non-Goals

- Do not rewrite all template instantiation or substitution machinery.
- Do not remove display names, dump keys, or diagnostic text for template specializations.
- Do not solve unrelated name-identity compatibility bridges unless they directly block the selected owner path.
- Do not add a named-case shortcut for one rendered template spelling.
- Do not weaken existing template record, layout, or frontend test expectations.

## Working Model

- Semantic owner identity should use structured facts when they are present.
- Rendered canonical specialization text is allowed as output and compatibility data, but not as primary semantic identity in the selected metadata-rich owner path.
- A metadata-rich owner path should reject, assert, or report a missing or mismatched structured identity instead of silently treating equal rendered text as equivalent.
- Compatibility-only surfaces may continue to carry display text when no semantic owner decision is being made.

## Execution Rules

- Keep each implementation packet bounded to one owner path or one immediately required helper extraction.
- Prefer existing identity data and ownership structures over adding a parallel identity system.
- Keep compatibility bridges visibly named as display or fallback surfaces.
- Add tests that exercise owner identity, record lookup, or layout ownership behavior, not only dump spelling.
- For code-changing packets, run a fresh build or compile proof and the supervisor-delegated focused CTest subset.
- Escalate to a plan review before broad template-system rewrites or expectation downgrades.

## Steps

### Step 1: Locate the Bounded Owner Path

Goal: Identify the smallest template record owner or layout-owner path where `SpecializationKey::canonical` currently influences semantic identity.

Primary target: `SpecializationKey`, `HirRecordOwnerTemplateIdentity`, and consumers in record lookup or layout ownership.

Actions:

- Inspect definitions and direct call sites for `SpecializationKey::canonical`.
- Trace one path from template specialization binding into record owner identity or aggregate layout ownership.
- Identify structured specialization facts already available on that path.
- Record in `todo.md` which path is selected and why it is bounded.

Completion check:

- The selected path is named with concrete files/functions.
- The executor can state which rendered string dependency will stop being semantic authority.
- Nearby same-feature cases are noted enough to avoid a testcase-shaped fix.

### Step 2: Thread Structured Owner Identity

Goal: Carry existing structured specialization identity into the selected owner path without broad machinery rewrites.

Primary target: The selected path from Step 1.

Actions:

- Add or reuse fields that carry structured specialization identity, owner indexes, or domain keys already available near the selected path.
- Keep rendered canonical text as display, dump, diagnostic, or compatibility payload.
- Name any compatibility fallback explicitly so it cannot be mistaken for semantic identity.
- Make missing structured data visible in metadata-rich paths through rejection, assertion, diagnostic reporting, or an equivalent explicit failure mode that matches local conventions.

Completion check:

- The selected owner identity path can make semantic decisions without depending primarily on `SpecializationKey::canonical`.
- A structured identity mismatch is not accepted merely because rendered canonical text matches.
- The change is limited to the selected owner path and immediately required helpers.

### Step 3: Preserve Output Compatibility

Goal: Keep existing user-facing and debug-facing rendered specialization text stable where it is only presentation data.

Primary target: HIR dumps, diagnostics, display names, and compatibility-only keys touched by Step 2.

Actions:

- Audit touched dump and diagnostic surfaces for accidental removal of rendered specialization strings.
- Keep display text visibly separate from semantic owner identity in names and data flow.
- Update only expectations that must change because the semantic owner path became more precise.

Completion check:

- Existing display and diagnostic behavior remains stable unless a focused semantic test requires a justified update.
- No test expectation is weakened to hide an unsupported path.
- Rendered strings are not reintroduced as semantic identity in the selected path.

### Step 4: Add Focused Proof

Goal: Prove that structured identity matters for the selected template record owner or layout-owner path.

Primary target: Focused HIR/frontend tests covering template record owner identity, record lookup, or layout ownership.

Actions:

- Add or update a focused test where equal or misleading display spelling is insufficient to prove owner identity.
- Include a nearby control case if needed to show display strings remain presentation payload.
- Avoid tests that assert only dump names, mangled names, or rendered spelling.

Completion check:

- Tests fail on the old canonical-string-authority behavior or directly cover the new explicit mismatch behavior.
- Tests cover owner identity, record lookup, or layout ownership semantics.
- No unsupported expectation downgrade is used as proof.

### Step 5: Validate and Summarize

Goal: Produce acceptance-quality proof for the bounded owner path.

Primary target: Fresh build or compile proof plus targeted HIR/frontend CTest coverage.

Actions:

- Run the supervisor-delegated build or compile command.
- Run the supervisor-delegated focused HIR/frontend CTest subset for the selected path.
- If the blast radius crosses multiple template identity buckets, request broader validation before closure.
- Summarize the selected path, semantic/display separation, tests, and remaining gaps in `todo.md`.

Completion check:

- Fresh build or compile proof is recorded.
- Focused CTest coverage is recorded.
- The source idea acceptance criteria are satisfied for the selected bounded owner path, or explicit blockers remain for a future lifecycle decision.
