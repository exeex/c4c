# Inherited Record Layout And Base-Member Access Todo

Status: Active
Source Idea: ideas/open/45_inherited_record_layout_and_base_member_access_followup.md
Source Plan: plan.md

## Active Item

- Step 1: Capture narrow repros for inherited aggregate initialization and
  plain derived-object access to inherited data members.

## Completed

- Activated `plan.md` from
  `ideas/open/45_inherited_record_layout_and_base_member_access_followup.md`.

## Next Slice

- identify the smallest existing inheritance-oriented test location
- add a failing regression for `Derived d{{1}};` preserving base storage
- add a failing regression for `derived.value` without an explicit cast

## Blockers

- none recorded

## Resume Notes

- keep the work limited to simple non-virtual base-subobject behavior
- if member lookup and layout require separate fixes, land them as separate
  tested slices
- do not absorb broader inheritance or cast-specific follow-on work into this
  runbook without a lifecycle transition
