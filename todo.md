Status: Active
Source Idea Path: ideas/open/97_structured_identity_completion_audit_and_hir_plan.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Follow-on idea drafting

# Current Packet

## Just Finished

Step 5 follow-on idea drafting materialized the idea 97 audit conclusions.

Drafting result:
- Idea 98 exists at `ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md` because the audit found meaningful parser/sema leftovers.
- Idea 98 is limited to parser helper overload leftovers plus sema enum variant mirrors, template NTTP/type-parameter validation mirrors, consteval NTTP binding mirrors, and type-binding text mirror cleanup.
- Idea 99 exists at `ideas/open/99_hir_module_symbol_structured_lookup_mirror.md`.
- Idea 99 starts HIR migration with the module function/global structured lookup mirror and preserves rendered names, mangled/template names, diagnostics, `LinkNameId`, and codegen/link output.
- `review/97_structured_identity_completion_audit.md` now names the concrete idea files and scopes instead of Step 5 placeholders.

## Suggested Next

Run Step 6 final audit consistency check. Re-read the review artifact against idea 97 acceptance criteria, confirm the idea 98 and idea 99 scopes remain separated, confirm no implementation/test files were modified, and prepare the supervisor lifecycle closure decision without closing idea 97 yet.

## Watchouts

- This runbook is audit and planning only; do not edit implementation files or tests.
- Leave unrelated files under `review/` alone; `review/parser_step8_demotion_route_review.md` was already untracked before this packet and was not touched.
- Idea 98 should stay parser/sema-only and must not absorb HIR module, type/tag, method/member, compile-time engine, or codegen cleanup.
- Idea 99 should not start with `TypeSpec::tag`, `Module::struct_defs`, method maps, or consteval constants. Those are later HIR slices after a module function/global mirror proves the route.
- Treat `LinkNameId` and rendered/mangled names as required bridge output for codegen/linking even after HIR source-key lookup mirrors exist.
- The first HIR slice may need declaration-side `Function::name_text_id` and `GlobalVar::name_text_id`, but that is HIR metadata preservation from existing AST fields, not a parser/sema rewrite.

## Proof

No build or test run. Step 5 was lifecycle/planning-only drafting from the completed parser, sema, and HIR audit classifications. No `test_after.log` was produced for this audit-only packet.
