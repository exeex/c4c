# Inherited Record Layout And Base-Member Access Todo

Status: Active
Source Idea: ideas/open/45_inherited_record_layout_and_base_member_access_followup.md
Source Plan: plan.md

## Active Item

- Step 4: Validate the simple single-base inheritance slice and record bounded
  follow-on work.

## Completed

- Activated `plan.md` from
  `ideas/open/45_inherited_record_layout_and_base_member_access_followup.md`.
- Added runtime regressions for inherited aggregate initialization and plain
  inherited member access.
- Fixed simple single-base aggregate initialization by lowering the first base
  subobject through a derived-to-base lvalue cast in local init-list handling.
- Fixed inherited member access in codegen by traversing base subobjects and
  emitting LLVM record layouts that include non-virtual base storage.
- Rebuilt from scratch, reran the full suite, and passed the regression guard
  with `before: 3153 passed, after: 3155 passed`.

## Next Slice

- extend inherited aggregate initialization beyond the current first-base
  zero-offset local-init lowering path if broader single-inheritance cases
  require it
- add focused coverage for inherited member access in method bodies, which
  still uses separate implicit-member lookup plumbing

## Blockers

- none recorded

## Resume Notes

- keep the work limited to simple non-virtual base-subobject behavior
- this slice intentionally targets simple single-base storage and local
  aggregate initialization; do not silently expand it into multi-base or
  virtual-base support
- do not absorb broader inheritance or cast-specific follow-on work into this
  runbook without a lifecycle transition
