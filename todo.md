# Execution State

Status: Active
Source Idea Path: ideas/open/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Draft Prepared And Debug Adapter Contracts
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed plan step 4, "Draft Prepared And Debug Adapter Contracts," by
replacing the step-1 placeholders in `prepared/` and `debug/` with explicit
adapter and proof-surface contracts that consume the canonical seams instead
of reopening lowering ownership locally.

## Suggested Next

Advance to plan step 5 by reviewing the full draft tree for manifest
completeness, dependency direction, and stage-4 implementation readiness, then
record that review in `docs/backend/x86_codegen_rebuild/review.md`.

## Watchouts

- The prepared layer is now explicitly query-shaped; step 5 should verify that
  no file in the draft tree slips back into broad prepared-context ownership
  or hidden lowering helpers.
- `prepared_fast_path_operands` must stay a translator over canonical frame
  and memory seams, not a rebadged local-slot subsystem.
- The debug layer must remain observational even when it mirrors matcher
  vocabulary; it should describe admission and fallback facts, not own them.

## Proof

Docs-only contract proof passed with:
`python3 - <<'PY' > test_after.log ...`
The check verified that the 7 step-4 `prepared/` and `debug/` files no longer
contain the step-1 placeholder marker and that each file records owned
inputs, owned outputs, allowed indirect queries, forbidden knowledge, and
role classification.
Proof log: `test_after.log`.
