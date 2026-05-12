# Identity Authority Migration Closure Gate

Status: Closed
Created: 2026-05-11
Closed: 2026-05-12

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

## Closure Notes

Closed after Step 5. The closure gate accepted the post-170 full-suite
baseline supplied by the supervisor: `3137/3137`, with the monotonic
regression guard passed and accepted. Guard usability was also proven by direct
guard execution, the guard self-test, and the CTest guard subset; the guard
reported 235 classified declaration-level hits.

Identity-domain closure ledger:
- `TextId` owns source-facing syntax spelling and source-origin text where
  semantic equality should not be reconstructed from rendered strings.
- Structured semantic keys own declaration, constant, slot, record, global,
  and route-state lookup equality in sema, HIR, LIR, BIR, and backend domains.
- `QualifiedNameKey` owns qualified semantic declaration identity when module
  or namespace path matters.
- Owner-aware template keys own template binding and canonical-symbol identity
  where rendered template shape must be paired with the owning context.
- `LinkNameId` owns final link-visible function/global symbol identity and
  separates source spelling from emitted link identity.
- Route-local ids own local slots, temporaries, blocks, memory bases, phi
  labels, string-pool names, prealloc/regalloc state, and out-of-SSA
  synchronization inside one lowering route.
- Display/output text owns diagnostics, debug dumps, printers, parity labels,
  final assembly text, LLVM text, and ABI spellings as output, not semantic
  lookup authority.

Remaining bridge status:
- No unclassified declaration-level semantic string-authority entries remain
  in the current inventory or idea 170 guard classification set.
- Retained bridges are classified by owner, domain, category, and reason, and
  are limited to parser/source spelling, structured semantic boundary gaps,
  qualified-name compatibility, owner-aware template compatibility,
  link-name/raw import compatibility, route-local synchronization, or
  output-only text.
- Bridge removal is gated on producers passing the corresponding typed
  syntax, semantic, qualified, owner-aware, link, or route-local id/key to the
  consumer. Output-only text only becomes a blocker if a future production
  lookup feeds it back into semantic identity.

Non-blocking follow-up candidates:
- AArch64 direct LIR emitter string-keyed lowering.
- The larger prealloc/out-of-SSA raw-name helper set.

These are not closure blockers. They are candidates only if a future route
chooses to migrate deeper backend-local helper state to typed route-local ids.

Result: close accepted. No new identity-authority follow-up idea is required to
close the string identity-authority migration.

## Reviewer Reject Signals

- The gate accepts a failing baseline without explaining whether failures are
  known, unrelated, or intentionally deferred.
- The guard exists but cannot be run by developers.
- The closure ledger handwaves remaining semantic string lookups.
- The idea expands into type identity or unrelated backend feature work.
