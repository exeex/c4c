# Execution State

Status: Active
Source Idea Path: ideas/open/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Draft Canonical Lowering Family Contracts
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed plan step 3, "Draft Canonical Lowering Family Contracts," by
replacing the step-1 placeholders in `lowering/` with explicit frame, call,
return, memory, comparison, scalar, float, and atomics/intrinsics ownership
contracts.

## Suggested Next

Advance to plan step 4 by drafting the `prepared/` and `debug/` adapter
contracts so the prepared route is explicitly constrained to consume the
canonical seams instead of acting like a second lowering stack.

## Watchouts

- The lowering drafts now name the semantic owners; step 4 must treat those as
  consumed services rather than reopening frame homes, call lanes, memory
  operands, or predicate policy inside prepared-facing files.
- `float_lowering` intentionally absorbs the legacy `f128`/x87 path so that
  step 4 does not invent a separate compatibility owner for long-double
  behavior.
- `atomics_intrinsics_lowering` remains canonical lowering despite containing
  ISA-specialized helpers; prepared adapters should not become another target-
  specific capability bucket.

## Proof

Docs-only contract proof passed with:
`python3 - <<'PY' > test_after.log ...`
The check verified that the 16 step-3 `lowering/` files no longer contain the
step-1 placeholder marker and that each file records owned inputs, owned
outputs, allowed indirect queries, forbidden knowledge, and role
classification.
Proof log: `test_after.log`.
