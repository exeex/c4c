Status: Active
Source Idea Path: ideas/open/prealloc-call-plan-phase-split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Call Plan Surface

# Current Packet

## Just Finished

Activated `ideas/open/prealloc-call-plan-phase-split.md` into `plan.md` and
initialized this execution-state skeleton.

## Suggested Next

Execute Step 1 from `plan.md`: audit the call-plan target files and record the
helper-family map, public-contract notes, prepared-printer mirror map,
extraction candidates, and explicit no-edit proof in this file before making
implementation edits.

## Watchouts

- Keep this slice behavior-preserving.
- Do not change ABI policy, argument/result placement, clobbering,
  preservation, indirect-callee handling, memory-return behavior, formal
  publication behavior, or prepared dump meaning.
- Keep `calls.hpp` as the aggregate public contract unless usage proves a
  smaller independently consumed boundary.
- Reject line-count-only extraction, target-shaped shortcuts, named-case
  handling, or printer label changes that alter printed meaning.

## Proof

Activation-only lifecycle change. Ran `git diff --check`: passed.
