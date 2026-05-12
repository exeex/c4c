# Identity Authority Migration Closure Gate

Status: Open
Created: 2026-05-11

Depends On:
- `ideas/closed/170_string_authority_regression_guard.md`

Parent Ideas:
- `ideas/closed/158_sema_validate_string_authority_audit.md`
- `ideas/closed/159_sema_consteval_domain_table_authority.md`
- `ideas/closed/160_sema_canonical_symbol_template_key_authority.md`
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`
- `ideas/closed/167_whole_codebase_string_authority_final_audit.md`
- `ideas/closed/168_compatibility_bridge_retirement.md`
- `ideas/closed/169_route_local_identity_domain_cleanup.md`
- `ideas/closed/170_string_authority_regression_guard.md`

## Goal

Close the identity-authority migration as a coherent milestone.

This is a checkpoint idea, not another broad cleanup pass. It should verify
that the parser/sema/HIR/LIR/BIR/backend identity migration has a stable proof
story, that the regression guard from idea 170 is usable, and that any
remaining string paths are either documented bridges, route-local domains, or
follow-up work outside the string-authority epic.

## Why This Idea Exists

The migration touched several identity domains:

- source spelling and syntax carriers through `TextId`
- qualified and owner-aware semantic keys
- sema validation, consteval, and canonical-symbol domains
- HIR template binding, rendered registry, and route-local identity domains
- final link-visible symbols through `LinkNameId`
- compatibility bridges and regression guardrails

After idea 170, continuing to add string-authority cleanup ideas by inertia
would blur the boundary between this epic and the next compiler capability
theme. This gate decides whether the migration is actually done.

## In Scope

- Run the accepted broad validation for the post-170 state, including the
  full suite unless an explicit baseline policy says otherwise.
- Verify the idea 170 guard runs locally and has documented allowlist behavior.
- Review the idea 167 audit inventory and confirm every remaining semantic
  string-authority item is either fixed, converted into a concrete follow-up,
  or explicitly out of scope for this epic.
- Review compatibility bridge documentation after idea 168.
- Review route-local identity documentation after idea 169.
- Produce a short closure ledger describing which identity domain owns which
  kind of name:
  `TextId`, structured semantic keys, `QualifiedNameKey`,
  owner-aware template keys, `LinkNameId`, route-local ids, and display text.
- Decide whether to close the migration with no new identity-authority work, or
  open a narrow follow-up for any remaining blocker.

## Out Of Scope

- Starting the next type-identity migration work.
- Large code refactors not required for closure proof.
- Reopening already closed parser/sema/HIR/backend ideas without a concrete
  regression or unhandled audit item.
- Treating route-local display strings as source/link identity bugs.

## Acceptance Criteria

- The post-170 state has an accepted validation/baseline result.
- The string-authority guard is runnable and documented.
- Remaining identity bridges have owner, limitation, and removal condition.
- No untracked semantic string-authority item remains without a follow-up
  decision.
- The identity-domain closure ledger is written into the lifecycle artifacts or
  closure note.

## Reviewer Reject Signals

- The gate accepts a failing baseline without explaining whether failures are
  known, unrelated, or intentionally deferred.
- The guard exists but cannot be run by developers.
- The closure ledger handwaves remaining semantic string lookups.
- The idea expands into type identity or unrelated backend feature work.
