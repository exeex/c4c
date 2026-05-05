# Step 6 Accumulated Identity Review

Active source idea path: `ideas/open/143_typespec_identity_normalization_boundary.md`

Chosen base commit: `94c7f13dba11d838a98b8f8e892baf2b3e313042`

Why this base: `94c7f13db` is the delegated Step 6 checkpoint after the Step 4 route review, and its subject/file set advances `todo.md` from the completed parser-disambiguation packet into Step 6 broad validation. The requested accumulated review is explicitly scoped to implementation since this checkpoint.

Commit count since base: 10

Reviewed diff: `94c7f13db..HEAD`

## Findings

No blocking findings.

1. Medium: `todo.md` names the canonical proof log as `test_after.log`, but the root worktree has no `test_after.log`.

   `todo.md:59` says the proof is in `test_after.log`, while the present root logs are `test_before.log` and `test_baseline.log`. The current `test_before.log` does contain the 1/6 focused Step 6 result described at `todo.md:62` through `todo.md:68`, so this is proof bookkeeping rather than evidence that the slice is untested. The supervisor should normalize canonical log state before treating this packet as acceptance-ready or before comparing regression logs.

2. Low: The parser/EASTL changes remain source-aligned and are not a named-case shortcut.

   `parse_injected_base_type()` now guarantees an EOF token for injected parses at `src/frontend/parser/impl/support.cpp:145`, and dependent `typename` owner reparses route through that helper at `src/frontend/parser/impl/types/declarator.cpp:671` and `src/frontend/parser/impl/types/declarator.cpp:847`. `parse_type_name()` now uses `TentativeParseGuard` at `src/frontend/parser/impl/types/declarator.cpp:2138`, which keeps failed probes from corrupting parser position. The new EASTL-shaped fixture in `tests/frontend/cpp_hir_parser_declarator_deferred_owner_metadata_test.cpp:240` is focused, but the implementation is keyed to injected parser state and tentative-parse restoration, not to the fixture spelling.

3. Low: The visible alias TypeSpec repair preserves the intended identity boundary.

   `parse_base_type()` records whether visible type resolution came from `VisibleNameSource::UsingAlias` at `src/frontend/parser/impl/types/base.cpp:2326`, then excludes that path from typedef source `tag_text_id` preservation at `src/frontend/parser/impl/types/base.cpp:2449`. This rejects fabricated visible lexical alias identity while preserving the older typedef/source preservation route for non-alias carriers. It does not introduce a rendered-tag consumer fallback.

4. Low: The LIR aggregate changes move toward shared normalization instead of consumer-local rediscovery.

   Call lowering removes duplicated local owner-key construction and calls `typespec_aggregate_owner_key()` plus `normalize_lir_aggregate_struct_name_id()` at `src/codegen/lir/hir_to_lir/call/args.cpp:22` and `src/codegen/lir/hir_to_lir/call/args.cpp:30`; the same shape appears in call-target lowering. Structured layout lookup now realigns the legacy layout declaration to the normalized LIR `StructNameId` at `src/codegen/lir/hir_to_lir/core.cpp:395`, and the reusable mirror normalizer lives at `src/codegen/lir/hir_to_lir/core.cpp:418`. The remaining rendered-text checks are mirror validation and explicit legacy compatibility, not a new semantic authority path.

5. Low: The const-init array field initializer fix is semantic and appropriately narrow.

   `ConstInitEmitter` now drops one array dimension before looking up the field aggregate at `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:861` and `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:886`. That matches the `&(array + n)->field` and `&array->field` semantics described in `todo.md:12` through `todo.md:18`; it does not weaken an expectation or recognize a named testcase.

## Alignment Judgments

Idea-alignment judgment: matches source idea

Runbook-transcription judgment: plan matches idea

Route-alignment judgment: on track

Technical-debt judgment: watch

Validation sufficiency: needs broader proof

Reviewer recommendation: continue current route

## Answer To Review Question

Yes. The accumulated Step 6 slices remain aligned with `ideas/open/143_typespec_identity_normalization_boundary.md`: parser fixes repair producer/probe boundaries, LIR changes centralize aggregate `StructNameId` mirror normalization, structured layout lookup consumes normalized identity, and the const-init array-field change fixes the element aggregate lookup rather than adding testcase-shaped code. I do not see expectation weakening, unsupported reclassification, printed-LLVM substring matching, or new consumer-local rendered-tag authority.

Continue Step 6 runtime/initializer cleanup. A split or plan rewrite is not justified on route-quality grounds right now; the next packet should stay narrow around the remaining invalid aggregate/global initializer shapes and should first normalize the canonical proof log state.
