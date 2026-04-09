# Inherited Record Layout And Base-Member Access Follow-Up

Status: Closed
Last Updated: 2026-04-09

## Goal

Track inherited-record layout and non-casted base-member access gaps that were
exposed while probing casted base-reference member access, without expanding
the active cast-follow-up plan into general inheritance bring-up.

## Why This Idea Exists

While drafting a narrow regression for `((Base&)derived).value`, the first
version also checked inherited initialization and plain `derived.value`
readback. That surfaced a broader issue set:

- `Derived d{{1}};` lowered as an empty `Derived` object rather than preserving
  the base subobject layout and initializer
- direct inherited member access (`derived.value`) failed during lowering
  because field lookup stayed on `Derived` instead of resolving the base member

Those are real bugs, but they are wider inheritance/layout work than the
current C-style-cast follow-up review should absorb.

## In Scope

- record-layout correctness for non-virtual base subobjects
- aggregate initialization that populates inherited base storage
- inherited field/member lookup on derived objects outside cast-specific flows
- focused runtime regressions that prove plain base-subobject reads/writes work

## Out Of Scope

- changing the active cast-review `plan.md` to absorb broad inheritance work
- virtual inheritance, access control, or multi-base ambiguity handling unless
  a narrowed activation later chooses those explicitly

## Activation Hint

Activate this idea if inherited layout or plain base-member lookup becomes the
next concrete blocker after the current cast-follow-up plan finishes, or sooner
if another active slice cannot be kept narrow without fixing broader base
subobject semantics.

## Completion

Completed: 2026-04-09

This slice is now implemented for the targeted simple single-base,
non-virtual inheritance cases:

- derived aggregate initialization preserves inherited base storage for the
  covered local init-list path
- plain inherited data-member access through the derived object now lowers
  correctly
- runtime regression coverage landed for inherited base initialization and
  direct inherited member access
- full-suite regression guard passed with `3153 -> 3155` and zero new failures

## Leftover Follow-On Work

- extend inherited aggregate initialization beyond the current first-base
  zero-offset local-init lowering path if broader single-inheritance cases
  require it
- add focused coverage for inherited member access inside method bodies, which
  still uses separate implicit-member lookup plumbing
- keep multi-base, virtual-base, and access-control work out of this closed
  slice unless activated as a separate idea
