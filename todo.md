Status: Active
Source Idea Path: ideas/open/97_structured_identity_completion_audit_and_hir_plan.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Parser completion audit

# Current Packet

## Just Finished

Step 2 parser completion audit classified the remaining parser string lookup paths in `review/97_structured_identity_completion_audit.md`.

Parser result:
- `legacy-proof`: value binding lookup, using value aliases, NTTP default cache keys, and template instantiation dedup keys are structured-first with legacy mirrors/fallbacks and mismatch counters.
- `parser-leftover`: public parser-support helper overloads still accept string-keyed const-int and typedef/type-compatibility maps.
- `bridge-required` / `blocked-by-downstream`: rendered compatibility spellings, `struct_tag_def_map`, `TypeSpec::tag`, and template mangled names remain required until downstream HIR/type/codegen identity is migrated.
- `diagnostic-only`: mismatch counters and compatibility spelling fields are audit/proof telemetry, not semantic lookup owners.

## Suggested Next

Run Step 3 sema completion audit using the existing sema inventory in `review/97_structured_identity_completion_audit.md`. Classify sema string/structured lookup surfaces into `bridge-required`, `diagnostic-only`, `legacy-proof`, `sema-leftover`, and `blocked-by-hir`, with special attention to consteval, enum/const-int bindings, overload lookup, record/member/static-member lookup, and canonical-symbol surfaces.

## Watchouts

- This runbook is audit and planning only; do not edit implementation files or tests.
- Leave unrelated files under `review/` alone; `review/parser_step8_demotion_route_review.md` was already untracked before this packet and was not touched.
- Parser-only evidence does not justify a broad new parser route after idea 95; the only parser-owned leftovers found are narrow helper overloads.
- Create idea 98 only if Step 3 finds meaningful parser/sema leftovers after sema classification.
- Always keep HIR migration scope in idea 99 separate from parser/sema completion work.
- HIR already has `LinkNameId`/`MemberSymbolId` handoff surfaces, but the dominant identity maps remain rendered-string keyed; Step 4 should avoid collapsing rendered codegen requirements into parser/sema leftover work.

## Proof

No build or test run. The delegated proof for Step 2 was source evidence only unless a parser leftover claim needed proof; no such claim needed runtime proof. No `test_after.log` was produced for this audit-only packet.
