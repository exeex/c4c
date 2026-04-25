Status: Active
Source Idea Path: ideas/open/97_structured_identity_completion_audit_and_hir_plan.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Sema completion audit

# Current Packet

## Just Finished

Step 3 sema completion audit classified the remaining sema string lookup paths in `review/97_structured_identity_completion_audit.md`.

Sema result:
- `legacy-proof`: local scopes, globals, functions, overload sets, non-enum const-int bindings, consteval function registry, and consteval interpreter locals have structured/text/key mirrors with comparison proof; locals are structured-preferred where a local key exists.
- `sema-leftover`: enum variant mirrors are declared but not populated; template NTTP/type-parameter validation placeholders remain string-only; consteval NTTP call bindings expose text/key slots but populate only the legacy name map; type-binding text mirror parameters are threaded but discarded.
- `blocked-by-hir`: struct completeness, record-key recovery, member/static-member lookup, out-of-class method owner setup, consteval record layouts, and `static_eval_int` still depend on `TypeSpec::tag` or HIR string-keyed struct/enum maps.
- `bridge-required` / `diagnostic-only`: canonical symbol/mangling spellings and consteval API compatibility strings are bridges; dual-lookup compare helpers and rendered diagnostic names are proof/diagnostic telemetry.

## Suggested Next

Run Step 4 HIR inventory and first-slice analysis using the existing HIR inventory in `review/97_structured_identity_completion_audit.md`. Keep parser/sema leftovers separate from HIR migration scope, and identify the first behavior-preserving HIR dual-lookup slice.

## Watchouts

- This runbook is audit and planning only; do not edit implementation files or tests.
- Leave unrelated files under `review/` alone; `review/parser_step8_demotion_route_review.md` was already untracked before this packet and was not touched.
- Parser-only evidence does not justify a broad new parser route after idea 95; the only parser-owned leftovers found are narrow helper overloads.
- Step 3 found meaningful sema leftovers that are specific enough for a possible idea 98: enum variant structured mirrors, template NTTP/type-parameter validation placeholders, consteval NTTP binding mirrors, and type-binding text mirror population.
- Always keep HIR migration scope in idea 99 separate from parser/sema completion work.
- HIR already has `LinkNameId`/`MemberSymbolId` handoff surfaces, but the dominant identity maps remain rendered-string keyed; Step 4 should avoid collapsing rendered codegen requirements into parser/sema leftover work.

## Proof

No build or test run. The delegated proof for Step 3 was source evidence only unless a sema leftover claim needed runtime proof; no such claim needed runtime proof. No `test_after.log` was produced for this audit-only packet.
