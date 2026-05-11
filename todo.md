# Current Packet

Status: Active
Source Idea Path: ideas/open/170_string_authority_regression_guard.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Guard Candidates

## Just Finished

Lifecycle activation created the runbook and execution scratchpad for Step 1.

## Suggested Next

Executor should start Step 1 by inventorying suspicious string-authority
patterns and choosing the first guard surface plus classification data shape.

## Watchouts

- Idea 171 depends on this guard; do not skip directly to migration closure.
- Idea 170 still mentions idea 168 as an open dependency, but the dependency is
  satisfied by `ideas/closed/168_compatibility_bridge_retirement.md`.
- The guard must not reject ordinary diagnostic, output, ABI spelling, or
  route-local string use.
- Do not claim progress from broad allowlist exceptions without owner, domain,
  category, and reason.

## Proof

Lifecycle-only activation. Required activation proof:
- `git diff --check -- plan.md todo.md`
