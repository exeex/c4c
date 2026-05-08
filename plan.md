# HIR LIR Rendered Owner Compatibility Retirement Runbook

Status: Active
Source Idea: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md

## Purpose

Retire remaining HIR and LIR rendered-owner compatibility routes after the parser-owned out-of-class owner-probe work from idea 151.

Goal: make HIR and LIR owner, aggregate, typedef, signature, and lowering paths prefer structured owner keys, qualifier `TextId` sequences, and record definitions before rendered name recovery.

## Core Rule

Rendered owner spelling is not semantic authority. It may remain only as display, diagnostics, or a documented compatibility fallback with an owner, limitation, and removal condition.

## Read First

- `ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/hir_lowering_core.cpp`
- `src/frontend/hir/impl/templates/member_typedef.cpp`
- `src/frontend/hir/impl/templates/type_resolution.cpp`
- `src/codegen/shared/llvm_helpers.hpp`
- `src/codegen/lir/hir_to_lir/core.cpp`
- `src/codegen/lir/hir_to_lir/lvalue.cpp`
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`
- Focused frontend/HIR/LIR tests that already mention qualifier metadata, deferred member type owners, aggregate layout, or structured layout observations.

## Current Targets

- HIR out-of-class method lowering that can recover owner identity by splitting rendered method or owner names after structured lookup misses.
- HIR owner-key construction that repairs parser/module text-table mismatches through rendered spelling.
- Aggregate `TypeSpec` owner/layout lookup in HIR and LIR, including `typespec_aggregate_owner_key`, `typespec_aggregate_compatibility_tag`, `find_typespec_aggregate_layout`, and `lookup_structured_layout`.
- HIR deferred member typedef and signature type recovery through incomplete `TypeSpec` owner metadata.
- LIR aggregate helper compatibility tags, structured layout observations, and owner recovery used after structured lookup misses.

## Non-Goals

- Do not reopen parser out-of-class owner-probe classification completed by idea 151.
- Do not remove diagnostic, dump, or legacy display names when structured owner metadata exists beside them.
- Do not treat one rendered `TextId` containing `A::B` as semantic compound-owner identity.
- Do not perform broad backend rewrites unrelated to aggregate owner/layout recovery.
- Do not weaken tests, mark tests unsupported, or rewrite expectations only to match changed rendered names.
- Do not hide the same rendered-owner fallback behind a renamed helper and claim retirement.

## Working Model

- Parser and HIR producers should carry complete `qualifier_text_ids`, `unqualified_text_id`, namespace/global metadata, and owner-indexed registration where needed.
- HIR owner keys should be created from semantic table data or already-canonical owner carriers, not by reinterpreting rendered spelling across text tables.
- Aggregate `TypeSpec` paths should prefer `record_def`, complete module-canonical owner keys, or owner-indexed layout declarations.
- Member typedef and signature recovery should use `deferred_member_type_owner_key`, qualifier `TextId`s, and member `TextId`s before any name recovery.
- LIR should receive complete structured aggregate names and owner keys from HIR module export before falling back to compatibility tags.

## Execution Rules

- Start by inventorying rendered-owner compatibility routes and classifying each as semantic authority, display/diagnostic, compatibility, or unrelated.
- Replace decision authority before deleting compatibility code.
- Keep any remaining compatibility fallback explicit with owner, limitation, and removal condition.
- Prefer structured owner-key or record-definition propagation over testcase-shaped matching.
- Every code-changing step needs fresh build or narrow compile/test proof chosen by the supervisor.
- Escalate validation when aggregate layout, ABI lowering, or shared HIR owner behavior changes.

## Step 1: Inventory HIR LIR Compatibility Authority

Goal: identify the rendered-owner compatibility routes that still affect HIR or LIR semantics and choose the first narrow implementation packet.

Primary targets:
- `src/frontend/hir/`
- `src/codegen/shared/llvm_helpers.hpp`
- `src/codegen/lir/hir_to_lir/`

Actions:
- Search for rendered owner spelling splits, compatibility tags, fallback owner lookup, `TypeSpec` name recovery, and cross-table spelling repair.
- Classify each finding as semantic authority, display/diagnostic, temporary compatibility, or unrelated.
- Map each in-scope compatibility route to one of the source idea owners: HIR lowerer, HIR owner-key bridge, HIR/LIR aggregate layout bridge, HIR member typedef resolver, or LIR lowering.
- Record the first implementation packet, owned files, and supervisor-delegated proof command in `todo.md` before code changes begin.

Completion check:
- `todo.md` names the concrete first implementation packet, owned files, and proof command.
- The inventory distinguishes in-scope semantic fallback from allowed display or diagnostic rendering.
- No source idea edit is needed unless the inventory contradicts durable source intent.

## Step 2: Retire HIR Method Rendered-Name Recovery

Goal: make HIR out-of-class method lowering rely on complete structured method owner metadata before rendered method-name fallback.

Actions:
- Verify parser and HIR function producers provide complete `qualifier_text_ids`, `unqualified_text_id`, namespace/global metadata, and owner-indexed method registration for the targeted path.
- Replace or demote any HIR method attachment logic that splits rendered `Node::name` after structured metadata is present.
- Keep diagnostic or dump rendering separate from semantic lookup.
- Add or update focused coverage for stale rendered-owner spelling and same-suffix owner ambiguity in out-of-class method lowering.

Completion check:
- HIR method attachment succeeds through structured metadata for the focused cases.
- Any remaining rendered method-name fallback is documented with owner, limitation, and removal condition.

## Step 3: Canonicalize HIR Owner-Key Carriers

Goal: remove parser/module text-table spelling repair as semantic authority when HIR can carry canonical owner keys directly.

Actions:
- Inspect owner-key helpers such as `make_ast_node_unqualified_text_id_for_owner_key` and nearby namespace qualifier construction.
- Identify where parser `TextId`s cross into module link-name text tables and require rendered spelling repair.
- Carry or convert canonical owner keys at the boundary instead of rebuilding meaning from rendered spelling.
- Preserve legacy rendered helpers only as compatibility or display helpers.

Completion check:
- HIR owner-key construction uses semantic owner carriers or module-canonical `TextId`s for the targeted path.
- Cross-table spelling repair no longer acts as the primary owner lookup route for that path.

## Step 4: Retire Aggregate TypeSpec Layout Fallbacks

Goal: make aggregate `TypeSpec` and layout lookup use structured owner keys or record definitions before name recovery.

Actions:
- Inspect `typespec_aggregate_owner_key`, `typespec_aggregate_compatibility_tag`, `find_typespec_aggregate_layout`, `lookup_structured_layout`, and their HIR producers.
- Fill missing `record_def`, owner key, `tag_text_id`, qualifier, or layout declaration metadata for the targeted aggregate route.
- Remove obsolete rendered compatibility lookup when all active callers provide structured metadata.
- Keep necessary legacy fallback explicit and secondary.

Completion check:
- Focused aggregate layout cases use structured owner/layout data rather than rendered tags.
- Validation covers the HIR/LIR/frontend subset touched by aggregate layout behavior.

## Step 5: Retire Member Typedef And Signature Recovery Fallbacks

Goal: make member typedef and signature type recovery use complete structured owner metadata before incomplete `TypeSpec` name recovery.

Actions:
- Inspect deferred member typedef producers and consumers around `deferred_member_type_owner_key`, qualifier `TextId`s, and member `TextId`s.
- Ensure deferred member typedef producers carry owner context, global qualification, qualifier segments, and member text identity.
- Replace semantic recovery from rendered owner/type names in the targeted path with structured owner-key lookup.
- Add or update focused coverage for stale owner spelling and nested member typedef recovery.

Completion check:
- Member typedef and signature recovery passes through structured metadata for the targeted cases.
- Remaining fallback, if any, is explicitly documented with owner, limitation, and removal condition.

## Step 6: Retire LIR Aggregate Helper Compatibility Tags

Goal: make LIR lowering consume complete structured aggregate names and owner keys from HIR module export before compatibility tag recovery.

Actions:
- Inspect LIR aggregate helper use in `src/codegen/shared/llvm_helpers.hpp` and `src/codegen/lir/hir_to_lir/`.
- Ensure HIR module export supplies complete structured aggregate owner/name data for the targeted lowering path.
- Remove or demote LIR compatibility tag lookup once structured lookup covers active callers.
- Keep structured layout observations focused on validation, not as a substitute for semantic ownership.

Completion check:
- LIR aggregate lowering uses structured owner/layout data for the targeted aggregate use.
- Any remaining compatibility tag route is documented as temporary and secondary.

## Step 7: Validate And Review Remaining Compatibility

Goal: prove the source idea acceptance criteria or leave a precise lifecycle note for remaining scoped work.

Actions:
- Run the supervisor-selected focused proof and any broader regression needed for changed HIR/LIR/frontend behavior.
- Re-inventory rendered owner splits, compatibility tags, and spelling-derived semantic lookup keys touched by this plan.
- Check reviewer reject signals from the source idea, especially renamed string authority, `TextId`-alone compound identity, expectation downgrades, parser route drift, and broad lowering rewrites.
- Record remaining compatibility owners, limitations, and removal conditions in `todo.md` for lifecycle review.

Completion check:
- Acceptance criteria from idea 152 are satisfied or the remaining in-scope routes are explicitly identified for the next lifecycle decision.
- Regression proof is fresh.
- No testcase-overfit or unsupported expectation downgrade is accepted as progress.
