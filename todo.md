Status: Active
Source Idea Path: ideas/open/97_structured_identity_completion_audit_and_hir_plan.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final audit consistency check

# Current Packet

## Just Finished

Step 6 final audit consistency check completed for idea 97.

Audit result:
- `review/97_structured_identity_completion_audit.md` records final acceptance status against idea 97.
- Parser findings remain separated into bridge-required, diagnostic-only, legacy-proof, parser-leftover, and blocked-by-downstream classifications.
- Sema findings remain separated into bridge-required, diagnostic-only, legacy-proof, sema-leftover, and blocked-by-HIR classifications.
- Bridge/downstream-blocked strings are not folded into parser/sema leftover scope.
- Idea 98 exists at `ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md` and is parser/sema-only.
- Idea 99 exists at `ideas/open/99_hir_module_symbol_structured_lookup_mirror.md` and is HIR module function/global mirror-only for the first slice.
- HIR cleanup remains separate from parser/sema completion work.

## Suggested Next

Suggested supervisor lifecycle decision: route to the plan owner for idea 97 closure/deactivation review, leaving idea 98 and idea 99 open as separate follow-on source ideas.

## Watchouts

- This runbook is audit and planning only; do not edit implementation files or tests.
- Leave unrelated files under `review/` alone; `review/parser_step8_demotion_route_review.md` was already untracked before this packet and was not touched.
- Idea 98 should stay parser/sema-only and must not absorb HIR module, type/tag, method/member, compile-time engine, or codegen cleanup.
- Idea 99 should not start with `TypeSpec::tag`, `Module::struct_defs`, method maps, or consteval constants. Those are later HIR slices after a module function/global mirror proves the route.
- Treat `LinkNameId` and rendered/mangled names as required bridge output for codegen/linking even after HIR source-key lookup mirrors exist.
- The first HIR slice may need declaration-side `Function::name_text_id` and `GlobalVar::name_text_id`, but that is HIR metadata preservation from existing AST fields, not a parser/sema rewrite.

## Proof

No build or test run. Step 6 was source/document consistency audit only.

Proof commands:
- `git status --short`
- `sed -n '1,240p' plan.md`
- `sed -n '1,260p' todo.md`
- `sed -n '1,260p' ideas/open/97_structured_identity_completion_audit_and_hir_plan.md`
- `sed -n '1,260p' ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md`
- `sed -n '1,280p' ideas/open/99_hir_module_symbol_structured_lookup_mirror.md`
- `sed -n '1,320p' review/97_structured_identity_completion_audit.md`
- `sed -n '260,620p' review/97_structured_identity_completion_audit.md`
- `find ideas/open -maxdepth 1 -type f | sort`
- `find review -maxdepth 1 -type f | sort`

No `test_after.log` was produced or required for this audit-only packet.
