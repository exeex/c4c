# Inherited Record Layout And Base-Member Access Runbook

Status: Active
Source Idea: ideas/open/45_inherited_record_layout_and_base_member_access_followup.md

## Purpose

Turn the inherited-record follow-up note into an execution runbook for the next
implementation slice after the current cast-specific work. Keep the effort
narrowly focused on non-virtual base-subobject layout, aggregate
initialization, and plain inherited member access.

## Goal

Make simple single-base inheritance preserve base storage layout and allow
plain derived-object access to inherited data members.

## Core Rule

Fix generic inheritance/layout behavior. Do not stretch this plan into virtual
inheritance, access-control review, multi-base ambiguity handling, or other
cast-specific follow-up work.

## Read First

- [ideas/open/45_inherited_record_layout_and_base_member_access_followup.md](/workspaces/c4c/ideas/open/45_inherited_record_layout_and_base_member_access_followup.md)
- [prompts/EXECUTE_PLAN.md](/workspaces/c4c/prompts/EXECUTE_PLAN.md)

## Current Targets

- inherited record layout for non-virtual base subobjects
- aggregate initialization that populates inherited base storage
- inherited field/member lookup on derived objects without explicit casts
- focused runtime regressions for plain base-subobject reads and writes

## Non-Goals

- virtual inheritance
- access-control enforcement changes
- multi-base ambiguity handling unless a narrow blocker proves unavoidable
- expanding the active slice into general inheritance bring-up beyond the cases
  named above
- reopening cast-specific behavior that already has its own plan lineage

## Working Model

The expected progression is:

1. reproduce the current failure modes with narrow tests
2. separate layout/initialization defects from member-lookup defects
3. fix one root cause at a time
4. validate with targeted inheritance tests before broader regression runs

## Execution Rules

- use test-first slices
- prefer generic frontend/HIR fixes over testcase-shaped behavior
- keep new regressions small and easy to compare against Clang
- if broader inheritance work appears necessary, record it in `ideas/open/`
  rather than silently expanding this runbook

## Step 1: Capture Narrow Repros

Goal: establish focused tests for the exact inherited-layout and plain
base-member-access failures.

Primary targets:

- `tests/`
- the smallest existing inheritance-related coverage points that match the bug

Actions:

- add or update a regression for aggregate initialization of a derived object
  whose only state lives in a non-virtual base
- add or update a regression for direct inherited member access through the
  derived object, such as `derived.value`
- confirm expected behavior with Clang when output shape is unclear

Completion check:

- there is a small test surface that fails for the current root causes without
  depending on cast-specific syntax

## Step 2: Repair Base-Subobject Layout And Initialization

Goal: make derived-object construction preserve base storage and initializer
flow for simple non-virtual inheritance.

Primary targets:

- record layout / object construction code touched by non-virtual base storage
  handling

Actions:

- inspect how derived records currently materialize base fields or storage
- fix the narrowest root cause that causes `Derived d{{1}};` to lose the base
  subobject payload
- keep the change limited to plain non-virtual base layout and initialization

Completion check:

- targeted initialization/layout regressions pass and show the base value is
  preserved in the derived object

## Step 3: Repair Inherited Member Lookup On Derived Objects

Goal: resolve plain inherited field access through a derived object without
requiring an explicit cast.

Primary targets:

- member/field lookup logic used for derived-object access

Actions:

- inspect where lookup currently stops at the derived record instead of walking
  base members
- implement the narrowest generic fix for inherited data-member lookup on
  simple non-virtual bases
- keep behavior aligned with the same layout assumptions validated in Step 2

Completion check:

- direct inherited reads/writes through `derived.member` pass in focused tests

## Step 4: Validate And Bound Follow-On Work

Goal: prove the slice is stable and record anything adjacent but out of scope.

Actions:

- rerun targeted inheritance tests plus nearby regression coverage
- rerun the broader repo validation required by `prompts/EXECUTE_PLAN.md`
- if additional inheritance gaps surface that are not required for this runbook,
  record them in the linked idea or a new open idea

Completion check:

- the focused inheritance slice is covered by tests
- broader validation is monotonic
- any newly discovered adjacent work is tracked without mutating this plan
