# Full X86 Backend Contract-First Replan

Status: Active
Source Idea: ideas/open/86_full_x86_backend_contract_first_replan.md
Supersedes: ideas/open/82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md
Activated from: ideas/open/86_full_x86_backend_contract_first_replan.md

## Purpose

Finish the contract-first x86 backend cleanup that still belongs to the root
replan idea now that the follow-on upstream seam work has been split out and
handled under ideas 87, 88, and 89.

## Goal

Leave idea 86 with an honest, compile-oriented contract/layout runbook:

- root backend and x86 ownership docs stay authoritative
- key x86 public and subsystem surfaces keep thin explicit seams
- in-place markdown companions describe live contract plus legacy evidence
- any remaining behavior recovery is tracked as separate work, not folded back
  into contract/layout packets

## Core Rule

Keep this route about contract/layout ownership. Do not reopen upstream seam
repair or x86 behavior recovery inside this runbook just because nearby work
recently landed.

## Read First

- [ideas/open/86_full_x86_backend_contract_first_replan.md](/workspaces/c4c/ideas/open/86_full_x86_backend_contract_first_replan.md)
- [src/backend/README.md](/workspaces/c4c/src/backend/README.md)
- [src/backend/mir/x86/README.md](/workspaces/c4c/src/backend/mir/x86/README.md)

## Current Targets

- root backend and x86 ownership contracts
- key x86 public seams that `backend.cpp` or other shared backend code consume
- in-place `*.cpp.md` / `*.hpp.md` companions that still read like extraction
  notes rather than stable design contracts
- durable separation between contract/layout work and later behavior recovery

## Non-Goals

- redoing the already-closed upstream seam ideas 87, 88, or 89 inside this plan
- fully restoring legacy x86 behavior in one packet
- hiding backend or BIR gaps behind x86-local workaround policy
- broad implementation churn outside the contract/layout surfaces named by the
  source idea

## Working Model

- the x86 tree should compile through thin public seams
- contract markdown is part of the subsystem interface, not passive notes
- behavior recovery packets must point at an existing contract boundary instead
  of inventing one while coding

## Execution Rules

- inspect the live tree first and treat the source idea's latest durable note
  as authoritative about follow-on seam ownership
- prefer small packets that either tighten one contract surface or confirm one
  compile-facing seam
- use `build -> focused proof -> broader validation only when blast radius
  expands` as the default proof ladder
- if execution discovers remaining work that is behavior recovery rather than
  contract/layout cleanup, record it as a separate idea instead of mutating
  this runbook

## Step 1: Audit Remaining Contract-First Gaps

Goal: identify the contract/layout surfaces still owned by idea 86 after the
seam-repair follow-ons moved out.

Primary targets:

- `src/backend/README.md`
- `src/backend/mir/x86/README.md`
- live x86 public headers and entrypoints
- key x86 markdown companions that define subsystem contracts

Actions:

- inspect the current x86 tree, ownership docs, and markdown companions
- separate true remaining contract/layout gaps from work already completed by
  ideas 87, 88, and 89
- pick the smallest contract-first packet that improves idea 86 without
  reopening behavior recovery

Completion check:

- the next implementation packet is concrete, contract-scoped, and does not
  duplicate closed follow-on seam work

## Step 2: Tighten Remaining Ownership And Markdown Contracts

Goal: promote any still-soft x86 contract surfaces into explicit ownership
documents and thin public seams.

Primary targets:

- subsystem READMEs or root contracts that still understate live ownership
- key `*.cpp.md` / `*.hpp.md` companions still missing durable contract content
- public x86 seams that remain broader than the contract-first tree intends

Actions:

- update the chosen docs or companions so they state ownership, inputs,
  outputs, deferred gaps, and legacy evidence clearly
- keep public seam changes narrow and aligned with the declared ownership graph
- avoid mixing in behavior fixes unless they are required to keep the contract
  surface honest

Completion check:

- the selected contract/layout surfaces read as durable interface documents
  rather than extraction artifacts or placeholder notes

## Step 3: Prove Live Interface Conformance

Goal: show the x86 tree still compiles through the contract-first seams after
the remaining ownership cleanup.

Primary targets:

- `backend.cpp`'s x86-facing includes or entrypoints
- touched x86 public surfaces
- compile or focused test proof proportional to the changed seams

Actions:

- run fresh build proof for the touched backend/x86 surfaces
- add focused validation when a contract change affects a concrete consumer
- escalate to broader validation only if the blast radius extends beyond one
  narrow seam

Completion check:

- proof shows the live x86 tree still routes through explicit thin seams and
  the contract/layout cleanup did not require hidden workaround paths

## Step 4: Separate Residual Behavior Recovery And Decide Closure

Goal: finish idea 86 without silently absorbing behavior work that belongs in a
different initiative.

Primary targets:

- any leftovers discovered during Steps 1 through 3
- source-idea success signal versus remaining real work

Actions:

- classify leftovers as either remaining contract/layout scope for idea 86 or
  separate behavior recovery
- create or switch to new ideas if residual work is adjacent but out of scope
- close idea 86 only when its contract/layout success signal is honestly met

Completion check:

- idea 86 is either ready to close on contract/layout grounds or has its
  remaining non-contract work clearly split into separate initiatives
