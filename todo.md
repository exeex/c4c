# Execution State

Status: Active
Source Idea Path: ideas/open/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Draft Canonical Entry, Core, ABI, And Module Contracts
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed plan step 2, "Draft Canonical Entry, Core, ABI, And Module
Contracts," by replacing the step-1 placeholders in `api/`, `core/`, `abi/`,
and `module/` with explicit ownership contracts covering inputs, outputs,
allowed indirect queries, forbidden knowledge, and dependency direction.

## Suggested Next

Advance to plan step 3 by drafting the canonical `lowering/` family contracts
so frame, call, return, memory, comparison, scalar, float, and
atomics/intrinsics ownership is explicit before the prepared adapters are
written.

## Watchouts

- The new shared seams must stay narrow when step 3 lands; lowering drafts
  should consume `core`, `abi`, and `module` contracts instead of recreating a
  mixed helper bucket.
- `module_emit` now explicitly delegates data rendering to
  `module_data_emit`; later drafts should preserve that separation instead of
  drifting back toward `prepared_module_emit.cpp` fusion.
- Prepared routes still must remain consumers of canonical lowering seams, so
  future drafts should avoid putting hidden stack-layout or call-lane policy
  back into prepared-facing files.

## Proof

Docs-only contract proof passed with:
`python3 - <<'PY' > test_after.log ...`
The check verified that the 11 step-2 draft files no longer contain the
step-1 placeholder marker and that each file records owned inputs, owned
outputs, allowed indirect queries, forbidden knowledge, and role
classification.
Proof log: `test_after.log`.
