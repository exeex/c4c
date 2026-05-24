Status: Active
Source Idea Path: ideas/open/prealloc-publication-accessor-contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Publication Accessor Surface

# Current Packet

## Just Finished

Activated `ideas/open/prealloc-publication-accessor-contracts.md` into
`plan.md` and initialized this execution-state skeleton.

## Suggested Next

Execute Step 1 from `plan.md`: audit the publication/accessor target files and
record the declaration map, include observations, stale-wording candidates, and
prepared-printer mirror map in this file before making implementation edits.

## Watchouts

- Keep this slice behavior-preserving.
- Do not split `module.hpp`, `regalloc.hpp`, or broad aggregate prealloc
  contracts.
- Do not make printer-only taxonomy changes unless they mirror real data-family
  naming or grouping changes.
- Reject broad codegen include churn, duplicate decode/home-selection logic, or
  semantic changes claimed as layout cleanup.

## Proof

Activation-only lifecycle change. Ran `git diff --check`: passed.
