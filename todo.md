Status: Active
Source Idea Path: ideas/open/97_structured_identity_completion_audit_and_hir_plan.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: HIR inventory and first-slice analysis

# Current Packet

## Just Finished

Step 4 HIR inventory and first-slice analysis grouped the HIR identity surfaces by subsystem in `review/97_structured_identity_completion_audit.md`.

HIR result:
- Module function/global lookup is the recommended first behavior-preserving HIR dual-lookup slice. It can use existing AST/HIR source-name metadata (`NamespaceQualifier`, qualifier text IDs, unqualified `TextId`) plus new HIR declaration-side name text IDs, while preserving rendered `name` and `LinkNameId` for codegen/link output.
- Struct/type tag and layout identity remains too broad for the first slice because `Module::struct_defs`, `struct_def_order`, codegen type declarations, and many `TypeSpec::tag` consumers still require rendered tags.
- Member/static-member/method lookup has useful `MemberSymbolId` and `field_text_id` surfaces, but owner identity is still tag-string based, so it should follow module symbol mirrors and struct/type-key groundwork.
- Function-local, template/compile-time registry, enum/const-int, and consteval environments all have mirror candidates, but each has either wider blast radius or parser/sema metadata dependencies.
- Downstream link/codegen handoff should preserve `LinkNameId` and rendered spelling as bridge-required output, not treat it as accidental semantic lookup.

## Suggested Next

Run Step 5 follow-on idea drafting. Create idea 98 for parser/sema leftovers found in Steps 2 and 3, and create idea 99 for the HIR module function/global dual-lookup starting slice recommended in Step 4. Keep idea 98 parser/sema-only and idea 99 HIR-only.

## Watchouts

- This runbook is audit and planning only; do not edit implementation files or tests.
- Leave unrelated files under `review/` alone; `review/parser_step8_demotion_route_review.md` was already untracked before this packet and was not touched.
- Parser-only evidence does not justify a broad new parser route after idea 95; the parser-owned leftovers found are narrow helper overloads.
- Step 3 found meaningful sema leftovers specific enough for idea 98: enum variant structured mirrors, template NTTP/type-parameter validation placeholders, consteval NTTP binding mirrors, and type-binding text mirror population.
- Idea 99 should not start with `TypeSpec::tag`, `Module::struct_defs`, method maps, or consteval constants. Those are later HIR slices after a module function/global mirror proves the route.
- Treat `LinkNameId` and rendered/mangled names as required bridge output for codegen/linking even after HIR source-key lookup mirrors exist.
- The first HIR slice may need declaration-side `Function::name_text_id` and `GlobalVar::name_text_id`, but that is HIR metadata preservation from existing AST fields, not a parser/sema rewrite.

## Proof

No build or test run. The delegated proof for Step 4 was source-evidence audit only; no HIR claim needed runtime proof. No `test_after.log` was produced for this audit-only packet.
