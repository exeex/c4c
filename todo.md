Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory the remaining `template_struct_*` string bridge callers

# Current Packet
## Just Finished
Replacement runbook activated after the prior three-step alias-template/member-follow-through plan was exhausted.

## Suggested Next
Execute Step 1 by classifying the remaining `template_struct_*` primary, specialization, and instantiation string-map callers before making code changes.

## Watchouts
- Keep the scope inside parser alias/template identity.
- Treat string-keyed template-struct maps as compatibility bridges unless inventory proves a use is still semantic.
- Do not widen into shared qualified-name infrastructure, backend, HIR, or repo-wide identity migration.
- Preserve existing parser behavior while changing the lookup substrate.

## Proof
Lifecycle-only reset; no code validation run.
