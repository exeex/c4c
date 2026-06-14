Status: Active
Source Idea Path: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Route 3 and x86 Memory Consumer Inventory

# Current Packet

## Just Finished

Activation created the runbook for Step 1 of the x86 Route 3 `LoadLocal`
source-memory agreement bridge.

## Suggested Next

Execute Step 1: inventory Route 3 `LoadLocal` memory/source records, prepared
`source_memory_access` rows, x86 prepared memory consumers, and candidate
reader/facade boundaries. Record the exact candidate and compatibility surfaces
in this file before implementation.

## Watchouts

- Agreement must compare a Route 3 record with the prepared source-memory row;
  prepared `memory_accesses` alone is not semantic authority.
- Preserve prepared lookup/status observability, helper/oracle names, fallback
  names, x86 output formatting, and target-owned addressing/register policy.
- Use x86-enabled validation if the focused x86 tests are not registered in the
  default build.

## Proof

Activation-only lifecycle change. Run `git diff --check -- plan.md todo.md`
before handoff.
