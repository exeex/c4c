# Phase F2 x86/riscv Prepared Boundary Completion Criteria Audit

Status: Active
Source Idea: ideas/open/248_phase_f2_x86_riscv_prepared_boundary_completion_criteria_audit.md

## Purpose

Define concrete completion criteria for the BIR/prepared ownership boundary
before x86/riscv portability work can safely connect shared semantic facts.

Goal: produce a reviewer-ready analysis result and bounded follow-up ideas for
the exact blocker families that still fail the criteria.

## Core Rule

This is analysis-only. Do not implement adapters, delete prepared fields,
demote public prepared APIs, open broad draft 155 retirement work, or claim
prepared aggregate retirement readiness from selected x86-only, riscv-only,
diagnostic-only, compatibility-only, or baseline-only proof.

## Read First

- `ideas/open/248_phase_f2_x86_riscv_prepared_boundary_completion_criteria_audit.md`
- `ideas/closed/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md`
- `ideas/closed/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md`
- `ideas/closed/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md`
- `ideas/closed/246_phase_f1_prepared_publication_status_compatibility_retention.md`
- `ideas/closed/247_phase_f1_final_prepared_field_group_demotion_gate.md`

## Current Targets

- Public lookup groups: `call_plans`, `memory_accesses`,
  `edge_publications`, `edge_publication_source_producers`.
- `PreparedBirModule` public/control fields: `module`, `names`,
  `control_flow`, `call_plans`, `store_source_publications`.
- Prepared helper/oracle/status/fallback/route-debug/wrapper-output
  compatibility surfaces.
- Target-policy surfaces for ABI, layout, registers, stack, storage,
  addressing, carriers, runtime helpers, formatting, emission, and wrapper
  behavior.
- Private pass context candidates: `route`, `invariants`,
  `completed_phases`, `notes`, and `liveness` if current inspection supports
  that classification.

## Non-Goals

- Do not modify backend implementation files.
- Do not rewrite expectations, baselines, helper names, or supported-path
  contracts as proof of ownership transfer.
- Do not move target policy into BIR.
- Do not hide public prepared authority behind private wrapper names.
- Do not create a broad "retire prepared" implementation idea.

## Working Model

Each fact family or field group must end with one explicit classification:

- BIR-owned semantic fact
- target-owned policy product
- private prepared pass context
- retained public compatibility adapter
- blocked public prepared authority
- deletion or demotion candidate

If a group is mixed, split it into smaller candidate rows. Do not leave
`mixed` as a final conclusion.

## Execution Rules

- Keep durable analysis and generated follow-up ideas under `ideas/open/`.
- Prefer one fact family or field-group sub-slice per follow-up idea.
- Each follow-up idea must say whether it is analysis-only, one-adapter
  implementation, compatibility-retention proof, x86/riscv parity proof,
  private-pass-context demotion, or deletion/demotion gate.
- Every generated idea must include concrete reviewer reject signals against
  named-case shortcuts, expectation weakening, classification-only progress,
  broad rewrites, and retained old failure modes.
- Keep draft 155 disposition explicit: keep blocked, rewrite, supersede, or
  retire obsolete.

## Step 1: Inventory Prior Closure Facts and Current Surfaces

Goal: establish the exact source material for the audit.

Primary targets:

- closure notes from ideas 243 through 247
- current declarations and consumers of the target lookup groups, module
  fields, helper/oracle/status/fallback surfaces, wrapper output, and debug
  strings

Actions:

- inspect closure notes from ideas 243 through 247 for durable blockers and
  proof already accepted
- inspect current source for the public prepared APIs and field groups named
  in this plan
- identify x86 consumers, riscv consumers, prepared compatibility consumers,
  and adjacent target-policy consumers for each group
- record findings in `todo.md` until they are ready to become durable output

Completion check:

- every current target has a named declaration surface and at least one named
  consumer bucket, or is explicitly recorded as unobserved with the search
  evidence used to decide that

## Step 2: Build Boundary Classification Matrix

Goal: classify each blocker group into explicit ownership buckets.

Actions:

- create a matrix row for each remaining blocker group from idea 247
- split any mixed row into smaller fact-family or field-group rows
- classify each row as BIR semantic, target policy, private pass context,
  retained compatibility, blocked public authority, deletion candidate, or
  demotion candidate
- explain why each row does or does not remain public prepared authority

Completion check:

- no row has only `mixed` as its conclusion, and every conclusion names the
  evidence or missing evidence that supports it

## Step 3: Define x86/riscv Portability Readiness Criteria

Goal: define what "clean enough to connect x86/riscv" means for every shared
fact family.

Actions:

- name the BIR or route fact that would be the semantic authority
- name the required x86 consumer and riscv consumer evidence
- name prepared agreement or fallback behavior that must remain stable
- require fail-closed coverage for missing, invalid, duplicate/conflict,
  mismatch, unsupported, prepared-only, fallback, and policy-sensitive cases
- keep ABI, layout, register, stack, storage, addressing, carrier/runtime
  helper, formatting, emission, and wrapper policy outside BIR ownership

Completion check:

- each candidate fact family is marked portable, partially portable, or
  blocked, with separate x86 and riscv evidence requirements

## Step 4: Define Prepared Public API Exit Criteria

Goal: decide the future exit path for each public prepared API or lookup group.

Actions:

- classify each public prepared API as retained compatibility, private pass
  context candidate, split target-policy plus BIR semantic index, deletion
  after consumer migration, or blocked
- name the observable condition that would make each transition safe
- call out compatibility strings, helper/oracle statuses, route-debug names,
  fallback names, and wrapper-output rows that must remain stable

Completion check:

- every public prepared API or lookup group has a stated exit path or a stated
  blocker that prevents an exit path today

## Step 5: Generate Follow-Up Ideas and Draft 155 Disposition

Goal: create bounded next executable packets only where the criteria are
concrete enough.

Actions:

- create one follow-up idea per fact family or field-group sub-slice that is
  ready for bounded execution
- mark each follow-up idea type explicitly
- include reviewer reject signals in every generated idea
- if no implementation packet is safe, create analysis-only blocker-map
  follow-ups instead of forcing demotion work
- record draft 155 disposition as keep blocked, rewrite, supersede, or retire
  obsolete

Completion check:

- every follow-up idea is narrow enough for a future agent to execute without
  broad route drift, and draft 155 has one explicit disposition

## Step 6: Package Closure Evidence

Goal: prepare the active idea for lifecycle closure without overstating
implementation progress.

Actions:

- add the final boundary classification matrix and operational completion
  criteria to the source idea's closure notes
- list portable, partially portable, and blocked x86/riscv fact families
- list prepared public APIs that remain compatibility authority
- list target-policy surfaces that must not move to BIR
- list private pass-context demotion candidates, if any
- summarize generated follow-up idea files
- run the close-time regression guard required by the lifecycle workflow

Completion check:

- the source idea has a reviewer-ready closure note, generated follow-up ideas
  exist where needed, and lifecycle close can be judged without relying on chat
  history
