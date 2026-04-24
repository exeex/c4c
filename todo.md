Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget alias-template registration away from spelling-shaped bookkeeping

# Current Packet
## Just Finished
Finished Step 2 by moving alias-template registration and lookup onto the structured `QualifiedNameKey` path, with only narrow parser-local fallback behavior left in place.

## Suggested Next
Start Step 3 by retargeting alias-template registration away from spelling-shaped bookkeeping so the metadata path follows the structured key family instead of rendered names.

## Watchouts
- Keep the work limited to the active parser/frontend scope.
- Keep registration and lookup aligned to the structured key path where it is already available.
- Preserve any remaining compatibility behavior as explicit parser-local bridge logic.

## Proof
Lifecycle repair only; no build or test validation run.
