# Grouped Register Bank And Storage Authority For Prealloc

Status: Active
Source Idea: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Activated from: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md

## Purpose

Turn grouped-register prealloc from an implicit scalar-only path into an
explicit target-independent authority layer for bank, span, alias, and storage
publication.

## Goal

Make prealloc publish truthful grouped physical-resource decisions so future
vector backends do not have to invent their own contiguous-register or alias
reasoning in downstream consumers.

## Core Rule

Repair grouped-resource semantics in prealloc itself. Do not hide the gap with
target-specific shortcuts, emitter-side inference, or testcase-shaped matching.

## Read First

- [ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md](/workspaces/c4c/ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md)
- [ideas/open/86_full_x86_backend_contract_first_replan.md](/workspaces/c4c/ideas/open/86_full_x86_backend_contract_first_replan.md)

## Current Targets

- prealloc authority surfaces that currently assume scalar physical assignment
- grouped register width and bank classification publication
- grouped occupancy and alias modeling used during candidate selection
- grouped save, spill, reload, and storage publication contracts
- regalloc proof coverage for width-aware occupancy and call-boundary behavior

## Non-Goals

- frontend type lowering for any concrete vector ISA
- target-specific register naming or final instruction emission
- turning liveness into a physical-resource phase
- unrelated scalar-only frame, stack, or call authority work

## Working Model

- liveness remains a lifetime fact source
- prealloc owns grouped physical-resource legality and publication
- downstream backends should consume published bank/span/storage facts rather
  than reconstructing contiguous-register rules themselves

## Execution Rules

- keep changes target-independent unless a proof surface requires an existing
  backend fixture
- prefer semantic authority extraction over one-off special cases
- keep packet boundaries small enough for build plus focused regalloc proof
- escalate to broader validation once shared prealloc contracts change across
  multiple consumers

## Step 1: Inspect Grouped-Resource Gaps And Authority Surfaces

Goal: identify the exact prealloc surfaces that still encode scalar-only
resource assumptions and map them to grouped bank/span/storage authority.

Primary targets:

- prealloc data models and contracts
- grouped register metadata producers and consumers
- existing regalloc tests that cover scalar occupancy or call clobbers

Actions:

- inspect current grouped register metadata, candidate selection, occupancy,
  and storage publication flows
- note which contracts already expose grouped width or bank facts and which
  still require downstream inference
- identify the smallest first semantic seam that unlocks grouped legality
  without pulling in target-specific emission work

Completion check:

- the owned grouped-resource gap is concrete enough to implement in one narrow
  packet without expanding scope

## Step 2: Add Grouped Bank And Span Authority To Prealloc

Goal: make prealloc publish grouped assignment legality in terms of bank,
width, span, and alias occupancy.

Primary targets:

- grouped candidate legality
- occupied-unit alias modeling
- publication of grouped physical assignment facts

Actions:

- extend the relevant prealloc contracts so grouped resources carry explicit
  bank/span authority
- ensure occupancy and alias checks reason about grouped units rather than
  scalar registers only
- keep downstream consumers reading published facts instead of recomputing
  contiguous-register legality

Completion check:

- grouped assignments are represented by shared prealloc semantics rather than
  consumer-side inference or testcase-specific handling

## Step 3: Publish Grouped Storage, Spill, Reload, And Clobber Semantics

Goal: connect grouped register authority to save, spill, reload, and
call-boundary publication paths.

Primary targets:

- grouped storage contracts
- grouped spill and reload publication
- grouped save and clobber publication around call boundaries

Actions:

- thread grouped storage authority through the prealloc publication surfaces
- ensure grouped clobber and preservation semantics respect bank/span facts
- keep scalar-only logic intact where grouped behavior is not involved

Completion check:

- grouped storage and call-boundary facts are published from prealloc without
  backend-specific reconstruction

## Step 4: Proof And Consumer Confirmation

Goal: prove the grouped-resource authority with focused tests and enough
consumer coverage to show downstream inference is no longer required.

Primary targets:

- narrow build proof
- focused regalloc tests
- broader validation if shared contracts or multiple consumers changed

Actions:

- run a fresh build or compile proof for the touched surfaces
- add or update regalloc tests that exercise grouped width-aware occupancy and
  call-boundary behavior
- escalate to broader validation when the blast radius reaches shared prealloc
  contracts

Completion check:

- proof demonstrates grouped bank/span/storage authority in prealloc and no
  backend still needs to invent contiguous-register legality on its own
