# Phase F4 Post-F3 Prepared Boundary Reassessment Gate

Status: Active
Source Idea: ideas/open/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md

## Purpose

Reassess the BIR/prepared boundary after the completed Phase F3 blocker maps,
agreement bridges, structural one-reader candidates, and baseline repair. The
output is a post-F3 boundary matrix plus bounded follow-up idea files for safe
next packets.

## Goal

Decide, row by row, whether any prepared field group, subfield, public lookup
group, compatibility surface, or private metadata field is now ready for a
narrow demotion/exit packet.

## Core Rule

This is an analysis-only gate. Do not edit implementation code, delete prepared
aggregates, open draft 155 as broad retirement work, or claim whole
`PreparedBirModule` / `PreparedFunctionLookups` retirement from narrow evidence.

## Read First

- `ideas/open/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md`
- Closure notes for ideas 248 through 263 named in the source idea.
- Current baseline artifacts, including the accepted full-suite status and any
  pending `test_baseline.new.log`.

## Current Targets

Classify every in-scope row named by the source idea:

- `PreparedFunctionLookups::call_plans`
- `PreparedFunctionLookups::memory_accesses`
- `PreparedFunctionLookups::edge_publications`
- `PreparedFunctionLookups::edge_publication_source_producers`
- `PreparedBirModule::module`
- `PreparedBirModule::names`
- `PreparedBirModule::control_flow`
- `PreparedBirModule::store_source_publications`
- `PreparedBirModule::route`
- `PreparedBirModule::invariants`
- `PreparedBirModule::completed_phases`
- `PreparedBirModule::notes`
- `PreparedBirModule::liveness`
- prepared helper/oracle/status/fallback/route-debug/printer/wrapper-output
  compatibility surfaces

Allowed classifications:

- ready for one concrete demotion/exit implementation idea
- ready only for another analysis blocker map
- retained compatibility authority
- target-policy-owned and not a BIR/prepared-retirement candidate
- private pass context already done
- still blocked by missing x86/riscv parity or missing fail-closed proof

## Non-Goals

- Do not directly edit compiler/backend implementation.
- Do not privatize, wrap, or delete prepared fields in this gate.
- Do not weaken expected strings, helper/oracle statuses, unsupported behavior,
  fallback behavior, wrapper output, prepared printer output, target output, or
  baseline tests.
- Do not create broad prepared retirement work.
- Do not move ABI, layout, register, stack, storage, addressing,
  carrier/helper, formatting, emission, instruction spelling, wrapper, or other
  target policy into BIR.

## Working Model

A fact family is thin enough to connect x86/riscv only when all of these hold:

- a single route/BIR semantic authority is named
- x86 consumes it through an agreement-gated path, or has explicit fail-closed
  non-applicability
- riscv consumes it through an agreement-gated path, or has explicit
  fail-closed non-applicability
- remaining public prepared surfaces are compatibility mirrors, not semantic
  sources of truth
- missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
  route-only, fallback, and policy-sensitive rows fail closed
- helper/oracle statuses, fallback names, route-debug output, prepared printer
  output, wrapper output, exact target output, and baseline behavior stay stable
- target-owned policy remains target-owned

If any row fails this standard, do not mark it demotion-ready. Name the exact
missing proof or consumer boundary.

## Execution Rules

- Treat old blocker language as stale until rechecked against the completed F3
  maps, bridges, structural one-reader candidates, and baseline repair.
- Compatibility strings and prepared helper/oracle statuses are evidence to
  preserve, not proof that semantic ownership transferred.
- Public prepared authority hidden behind a private accessor name is not a
  successful exit unless the row is removed, explicitly retained, or proven as
  a compatibility mirror.
- Follow-up ideas must be bounded to one fact family, one lookup group, one
  field sub-slice, or one private metadata field.
- When creating follow-up ideas, include concrete reviewer reject signals that
  block testcase-shaped shortcuts, expectation weakening, classification-only
  claims, broad rewrites, and old failure modes behind renamed abstractions.

## Steps

### Step 1: Gather Post-F3 Evidence

Goal: build the evidence inventory required by the gate before classifying any
row.

Concrete actions:

- Read closure notes for ideas 248 through 263 listed in the source idea.
- Record the relevant completed F3 facts for route 6 call identity, route 3
  memory/source parity, route 4/5 edge publication parity, compatibility
  fail-closed proof, private pass context metadata, liveness authority, structural
  one-reader candidates, and full-suite baseline repair.
- Verify whether the accepted baseline is still 3428/3428 and whether any
  `test_baseline.new.log` regression candidate is pending.

Completion check:

- `todo.md` contains a concise evidence inventory and baseline status with no
  row classifications yet claimed as final.

### Step 2: Build the Boundary Matrix

Goal: classify every in-scope row against the post-F3 evidence.

Concrete actions:

- For each prepared lookup group, prepared module field, and compatibility
  surface, identify the current semantic authority, consumers, compatibility
  mirrors, and fail-closed behavior.
- Assign one allowed classification to every row.
- For blocked rows, name the exact missing x86/riscv parity, consumer boundary,
  or compatibility proof.

Completion check:

- A complete post-F3 boundary matrix exists in `todo.md` or a supervisor-approved
  analysis artifact, with every in-scope row represented exactly once.

### Step 3: Apply the Thin-Enough Gate

Goal: reject unsafe demotion candidates before creating follow-up work.

Concrete actions:

- Recheck every demotion-ready candidate against the thin-enough standard.
- Confirm x86 and riscv evidence or explicit fail-closed non-applicability.
- Confirm retained compatibility behavior for helper/oracle statuses, fallback
  names, route-debug output, prepared printer output, wrapper output, target
  output, and baseline behavior.
- Separate target-policy-owned rows from BIR/prepared-retirement candidates.

Completion check:

- The matrix clearly distinguishes demotion-ready rows, analysis-only blockers,
  retained compatibility authority, target policy, private pass context, and
  still-blocked public prepared authority.

### Step 4: Decide Draft 155 and Follow-Up Shape

Goal: turn the matrix into bounded next-work decisions without broad retirement.

Concrete actions:

- List rows thinner than they were after idea 247.
- List rows still blocked and the exact missing proof.
- List rows that should remain target policy and never move to BIR.
- List rows already private pass context or ready for a private pass-context
  packet.
- Decide draft 155 disposition: keep blocked, rewrite, supersede, or retire
  obsolete.

Completion check:

- `todo.md` records the draft 155 disposition and the intended bounded follow-up
  set before lifecycle files are created.

### Step 5: Create Bounded Follow-Up Ideas

Goal: write only safe, bounded source ideas for the next packets.

Concrete actions:

- Create one follow-up idea per safe implementation or analysis packet.
- Keep each follow-up limited to one fact family, lookup group, field sub-slice,
  or private metadata field.
- If no implementation packet is safe, create only analysis follow-ups.
- Include concrete reviewer reject signals in each new idea.

Completion check:

- Follow-up idea files exist under `ideas/open/` for each safe next packet, and
  none authorizes broad prepared retirement or testcase-overfit work.

### Step 6: Close or Reassess the Gate

Goal: complete the active gate only when the source idea output requirements are
satisfied.

Concrete actions:

- Ensure the closure note includes the complete boundary matrix, thinner rows,
  blocked rows with missing proofs, target-policy rows, private pass-context
  rows, follow-up idea list, and draft 155 disposition.
- Ask the plan owner to close the active idea only after the source idea
  completion standard is met.
- If the runbook is exhausted but the source idea is not satisfied, request a
  lifecycle rewrite or split instead of closure.

Completion check:

- The source idea can be closed only after the required output exists and the
  close-time lifecycle gate accepts it.
