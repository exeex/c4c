Status: Active
Source Idea Path: ideas/open/92_aarch64_calls_owner_subresponsibility_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build Calls Owner Inventory

# Current Packet

## Just Finished

Activation created the canonical runbook and execution-state skeleton for `plan.md` Step 1.

## Suggested Next

Execute Step 1 by building a function-level inventory of `src/backend/mir/aarch64/codegen/calls.cpp` and `src/backend/mir/aarch64/codegen/calls.hpp`, with read-only comparison against the closed prerequisite ideas named in `plan.md`.

## Watchouts

- This route is audit-only; do not edit implementation files.
- Keep routine findings here in `todo.md` until a durable follow-up idea is justified.
- Reject broad "shrink calls.cpp" or vague "move to BIR" routes without a missing target-neutral fact and focused proof route.

## Proof

No validation run required for activation-only lifecycle changes.
