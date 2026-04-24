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

- implement the missing width-aware grouped allocation behavior using the
  published bank/span legality
- repair spill, reload, and call-boundary handling where grouped width `> 1`
  values still fall back to non-authoritative behavior
- add focused proof that grouped allocator output is consumed without backend
  policy recovery

Completion check:

- width `> 1` grouped values allocate and survive spill/reload and call-boundary
  cases through published allocator behavior rather than hidden special cases

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
