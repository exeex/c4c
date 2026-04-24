# Advanced Prepared Call Authority And Grouped-Width Allocation

Status: Active
Source Idea: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Activated from: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md

## Purpose

Continue the target-independent prepared-authority route after ideas 88 and 89
by finishing advanced call publication and making grouped width-greater-than-one
allocation behave as real allocator authority.

## Goal

Leave idea 91 with an honest prepared/regalloc runbook:

- advanced call families publish truthful prepared authority
- grouped width `> 1` values allocate, spill, reload, and cross call boundaries
  through published allocator behavior
- backend consumers can read the published facts directly instead of recreating
  policy locally

## Core Rule

Keep this route target-independent. Do not hide missing prepared or regalloc
authority behind x86-local recovery, testcase-shaped shortcuts, or expectation
downgrades.

## Read First

- [ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md](/workspaces/c4c/ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md)
- [ideas/closed/88_prepared_frame_stack_call_authority_completion_for_target_backends.md](/workspaces/c4c/ideas/closed/88_prepared_frame_stack_call_authority_completion_for_target_backends.md)

## Current Targets

- advanced prepared call families such as variadic, byval, indirect-result, and
  memory-return publication
- before-call and after-call move publication for harder call-boundary cases
- grouped width `> 1` allocation, spill, reload, and call-boundary behavior
- prepared and allocator dumps/tests that prove backends can consume the
  published authority directly

## Non-Goals

- x86-specific spelling, lowering recovery, or backend-local ABI policy
- frontend type-lowering expansions unrelated to the owned failure family
- out-of-SSA critical-edge or parallel-copy deepening from idea 90
- reworking liveness into physical-resource modeling

## Working Model

- prepared output should publish final call/frame authority for the covered
  advanced families
- grouped bank/span legality is already substrate; this plan turns width `> 1`
  allocation into truthful allocator behavior
- downstream backends should consume prepared and regalloc output without
  reconstructing missing call or grouped-allocation policy

## Execution Rules

- inspect the current prepared and prealloc surfaces first and keep the source
  idea's owned failure family authoritative
- prefer small packets that publish one missing authority seam or one grouped
  allocator behavior family at a time
- use `build -> focused proof -> broader validation only when blast radius
  expands` as the default proof ladder
- if execution reveals a distinct initiative outside advanced prepared call
  authority or grouped width allocation, record it as a separate idea instead
  of stretching this runbook

## Step 1: Audit Remaining Advanced Prepared-Authority Gaps

Goal: identify the smallest truthful next packet inside idea 91 after ideas 88
and 89.

Primary targets:

- prepared call-authority publication surfaces
- grouped width-aware allocator surfaces and dumps
- existing tests or fixtures covering advanced call families and grouped
  resource behavior

Actions:

- inspect the current prepared and prealloc implementation plus nearby proof
  coverage
- separate already-published scalar/group substrate behavior from the missing
  advanced call and width `> 1` authority
- choose the next packet that repairs the first bad target-independent fact

Completion check:

- the next implementation packet is concrete, target-independent, and tied to
  the owned failure family rather than to one named testcase

## Step 2: Publish Advanced Prepared Call Authority

Goal: make the remaining advanced call families publish truthful prepared
authority instead of relying on backend reconstruction.

Primary targets:

- prepared call-plan publication for variadic, byval, indirect-result, and
  memory-return families
- before-call and after-call move publication for advanced call boundaries
- dumps and fixtures that expose the new prepared facts directly

Actions:

- strengthen the prepared data and publication paths for the selected advanced
  call families
- make before/after-call move ownership explicit where advanced cases need it
- add focused proof that the prepared output carries the needed authority

Completion check:

- covered advanced call cases publish enough prepared authority that downstream
  code does not need backend-local ABI reconstruction for those facts

## Step 3: Make Grouped Width-Greater-Than-One Allocation Truthful

Goal: turn grouped width `> 1` allocation from substrate scaffolding into real
allocator behavior.

Primary targets:

- width-aware grouped allocation decisions
- grouped spill/reload publication
- grouped call-boundary and preserved/clobbered behavior

Actions:

- execute the numbered substeps below in order
- keep each packet focused on one grouped allocator truthfulness seam at a time
- reject scalar base-register reconstruction when published grouped span
  authority already exists

Completion check:

- all Step 3 substeps are complete
- width `> 1` grouped values allocate and survive spill/reload and call-boundary
  cases through published allocator behavior rather than hidden special cases

### Step 3.1: Make Grouped Call-Boundary Consumers Read Published Span Authority

Goal: finish the first downstream consumer seam after grouped call-plan span
publication so grouped call-boundary handling stops reconstructing ABI lanes
from scalar base-register identity.

Primary targets:

- grouped call-boundary consumer surfaces that still read scalar register names
  instead of grouped span metadata
- prepared/regalloc dumps and focused backend fixtures for grouped
  argument/result boundaries

Actions:

- replace grouped call-boundary consumer logic that still keys on scalar
  base-register identity with the published contiguous-width and occupied-span
  authority
- keep grouped preserved/clobbered and movement ownership anchored to the
  shared call-plan span fields instead of backend-local heuristics
- add focused proof that grouped call-boundary consumers can follow the
  published span directly in representative grouped argument/result cases

Completion check:

- grouped call-boundary consumers read published span authority directly
- no owned grouped boundary case still depends on scalar lane reconstruction

### Step 3.2: Make Width-Aware Grouped Allocation Decisions Truthful

Goal: ensure width `> 1` grouped values are allocated as real contiguous spans
through allocator decisions instead of scalarized placeholders.

Primary targets:

- grouped width-aware allocation decisions
- grouped preserved/live-through decisions outside the already-published
  call-plan seam
- focused allocator dumps and fixtures that expose chosen grouped spans

Actions:

- implement the missing width-aware grouped allocation decisions using the
  published bank/span legality as allocator authority
- keep grouped preserved/live-through handling tied to whole-span ownership
  rather than scalar alias reconstruction
- add focused proof that grouped allocator output shows truthful contiguous
  span choices for width `> 1` values

Completion check:

- width `> 1` grouped values receive truthful contiguous allocation decisions
- grouped allocator consumers do not need to recover allocation policy locally

### Step 3.3: Make Grouped Spill And Reload Publication Truthful

Goal: repair grouped spill/reload behavior so width `> 1` values survive
storage transitions through published allocator behavior rather than scalar
surrogates.

Primary targets:

- grouped spill/reload publication
- grouped storage and reload moves for width `> 1` values
- focused backend/regalloc proof covering grouped allocation plus spill/reload

Actions:

- repair spill and reload handling where grouped width `> 1` values still fall
  back to non-authoritative scalar-shaped behavior
- keep grouped storage ownership and reload movement explicit in allocator
  output and dumps
- add focused proof that grouped values survive allocation, spill/reload, and
  call-boundary cases without backend policy recovery

Completion check:

- grouped width `> 1` values survive spill/reload through published allocator
  behavior
- dumps and focused tests expose grouped storage transitions directly

## Step 4: Prove Consumer Use And Decide Closure

Goal: confirm the published authority is actually consumable and decide whether
idea 91 is complete or needs follow-on work.

Primary targets:

- prepared and regalloc dumps for the covered advanced families
- backend-consumption proof for the touched authority surfaces
- remaining gaps discovered during Steps 1 through 3

Actions:

- run fresh build proof and focused tests for the changed prepared/regalloc
  surfaces
- escalate to broader validation if the blast radius extends beyond one narrow
  bucket
- classify leftovers as remaining idea-91 scope or a separate initiative

Completion check:

- the covered advanced call and grouped width-allocation authority is published
  truthfully, consumed directly, and any residual work is honestly split rather
  than silently absorbed
