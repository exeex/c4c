# Phase F0 x86/riscv BIR Portability Convergence Audit

Status: Active
Source Idea: ideas/open/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md

## Purpose

Audit the post-E5 state before prepared/BIR thinning can claim that x86 and
riscv are portable over the same BIR semantic facts.

## Goal

Produce a durable convergence map and follow-up ideas for every remaining
blocker or safe final packet.

## Core Rule

This runbook is analysis-only. Do not implement adapter work, open draft 155,
delete prepared aggregates, weaken expectations, or claim prepared retirement
readiness directly.

## Read First

- ideas/open/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md
- ideas/closed/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md
- ideas/closed/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md
- ideas/closed/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md
- ideas/closed/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md

## Current Targets

- x86/riscv BIR semantic fact parity.
- Remaining public `PreparedFunctionLookups` and prepared lookup-group
  surfaces.
- Target wrapper policy boundaries for ABI, layout, register, stack, emission,
  formatting, and wrapper decisions.
- Diagnostic, helper-oracle, route-debug, expected-string, fallback, and
  baseline authority.
- `PreparedBirModule` field groups that are duplicate semantic cache, private
  pass context, target-policy product, or blocked public authority.

## Non-Goals

- Do not directly implement any retirement, adapter, demotion, or deletion
  packet.
- Do not broadly delete `PreparedBirModule` or `PreparedFunctionLookups`.
- Do not claim riscv readiness from x86-only evidence.
- Do not move ABI, layout, register, stack, emission, formatting, or wrapper
  policy into BIR.
- Do not weaken expected strings, helper-oracle statuses, supported-path
  contracts, fallback behavior, route-debug output, wrapper output, or baseline
  tests.

## Working Model

- BIR owns target-neutral semantic facts.
- Route facts, indexes, and identities should converge toward BIR ownership.
- Prepared owns target-specific or emission-specific policy that still consumes
  BIR-owned facts.
- Prepared APIs should become private pass context instead of durable public IR.
- Duplicate semantic helpers are deletion candidates only after their
  BIR-owned replacements are proven across x86 and riscv behavior.

## Execution Rules

- Inspect code, tests, and closure notes as evidence, but keep routine findings
  in `todo.md` until the closure map or follow-up ideas are ready.
- Create follow-up ideas under `ideas/open/` only after the fact family or
  blocker is concrete enough to be independently reviewed.
- Each follow-up idea must include concrete reviewer reject signals.
- Preserve string, fallback, diagnostic, oracle, route-debug, and baseline
  behavior as explicit proof requirements.
- Treat broad retirement claims, testcase-shaped shortcuts, and expectation
  weakening as route failures.
- Analysis-only packets may use targeted source/test inspection instead of a
  compiler build, but any generated implementation follow-up must include its
  expected proof ladder.

## Steps

### Step 1: Establish the Post-E5 Evidence Base

Goal: collect the closure evidence and current surfaces that define the audit
starting point.

Primary targets:

- The four E5 and adapter closure notes listed in Read First.
- Current `PreparedBirModule`, `PreparedFunctionLookups`, route identity,
  source identity, diagnostic, oracle, fallback, and target wrapper surfaces.

Actions:

- Inspect the closure notes and summarize what each proved, blocked, or left
  target-specific.
- Identify whether E4 follow-up 238 closed before E5 ran or remains a blocker.
- Locate the current public prepared lookup and field-group surfaces that still
  act as semantic authority.
- Record unresolved ambiguity in `todo.md` rather than editing the source idea.

Completion check:

- `todo.md` names the evidence files inspected, the current blocker categories,
  and the concrete surfaces that the next step must classify.

### Step 2: Audit x86/riscv BIR Fact Parity

Goal: decide which semantic facts are already portable, missing, unconsumed, or
target policy.

Primary targets:

- x86-selected route facts.
- riscv consumers that infer route identity, source identity, block identity,
  or producer/consumer relationships from prepared shape.
- BIR indexes or rows that could become target-neutral semantic authority.

Actions:

- Compare x86 and riscv consumption for route identity, source identity, block
  identity, memory source identity, edge publication identity, scalar call-use
  source identity, and producer/consumer relationships.
- Mark each fact family as portable BIR fact, missing BIR fact, unconsumed BIR
  fact, target-policy fact, or blocked by diagnostics/fallback/tests.
- Do not treat x86-only proof as riscv parity.

Completion check:

- The convergence map has an x86/riscv parity section with one classification
  per fact family and a follow-up/blocker disposition for each gap.

### Step 3: Classify Prepared Lookup and Field Groups

Goal: decide where each remaining prepared public lookup or field group belongs.

Primary targets:

- `PreparedFunctionLookups` public APIs.
- Prepared lookup groups and `PreparedBirModule` field groups.
- Consumers that still observe prepared naming, status, route-debug, fallback,
  or diagnostic behavior.

Actions:

- Classify each lookup or field group as private pass context, BIR-owned index,
  target-policy product, compatibility adapter, deletion candidate, or blocker.
- Separate duplicate semantic caches from required target policy.
- Preserve compatibility requirements for fallback behavior and status strings.

Completion check:

- The convergence map groups prepared surfaces by destination and explains why
  blocked public authority cannot move yet.

### Step 4: Split Target Policy from Semantic Authority

Goal: identify wrapper code that should consume BIR facts without moving target
policy into BIR.

Primary targets:

- x86 and riscv wrapper code.
- ABI, layout, register, stack, emission, formatting, and wrapper decisions.
- Target-local adapters required during convergence.

Actions:

- Find wrapper or target code that re-derives semantic route/source facts from
  prepared shape.
- Mark pure target policy separately from semantic fact derivation.
- Identify target-specific follow-up ideas that consume BIR facts while keeping
  policy target-owned.

Completion check:

- The convergence map has an explicit target-policy section and does not assign
  ABI/layout/register/stack/emission/wrapper policy to BIR.

### Step 5: Audit Diagnostic, Oracle, and String Authority

Goal: identify remaining prepared-shape proof points that must move to BIR
facts without weakening observed behavior.

Primary targets:

- Diagnostic rows, helper-oracle names and statuses, route-debug output,
  expected strings, fallback tests, and baseline tests.

Actions:

- List tests or debug rows that still prove behavior by observing prepared
  structure rather than BIR facts.
- Identify which names, statuses, and strings must remain stable during
  transition.
- Define proof requirements for transferring authority only after both x86 and
  riscv prove the same semantic fact.

Completion check:

- The convergence map contains diagnostic/oracle replacement candidates with
  explicit string-stability and baseline requirements.

### Step 6: Write Follow-Up Ideas and Closure Map

Goal: convert the audit into durable lifecycle artifacts without expanding this
analysis-only runbook into implementation.

Primary targets:

- New focused files under `ideas/open/` for missing portable facts,
  target-specific blockers, prepared lookup demotion packets, wrapper split
  packets, diagnostic/oracle replacements, and final field-group deletion or
  demotion packets.
- The source idea closure note when the audit is complete.

Actions:

- Write one follow-up idea per missing fact family, lookup group, target-policy
  split, diagnostic/oracle replacement, or safe field-group deletion/demotion.
- Include concrete in-scope, out-of-scope, acceptance, proof, and reviewer
  reject signals in each follow-up idea.
- Produce the post-E5 convergence map grouped by BIR semantic facts, prepared
  pass context, target policy, diagnostics/oracles, and deletion candidates.
- State criteria for prepared aggregate retirement and for rewriting,
  superseding, blocking, or retiring draft 155.

Completion check:

- The source idea can be closed only when the convergence map and follow-up
  ideas cover every remaining blocker or safe final packet, and no implementation
  changes were made as part of this audit.
