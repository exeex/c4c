# Step 6 Visible Alias Metadata Review

Active source idea path: `ideas/open/143_typespec_identity_normalization_boundary.md`

Chosen base commit: `3fc5888da0eb27cca5972a1d57a344b390317aec`

Why this base: `3fc5888da` is the latest material active-idea checkpoint for idea 143 and contains the Step 4 dependent `typename`/cast identity repair that this review must preserve. The requested visible-alias repair is isolated in the later slice `e6c26a847^..HEAD`, but the review base for route alignment remains the last source-idea checkpoint.

Commit count since base: 3

Focused reviewed slice: `e6c26a847d972e4196aa0f00d722db1ea13d57d2..941e28616ee82732b6f8b72632842d27123b5b11`, plus the implementation diff from `e6c26a847^..e6c26a847`.

## Findings

No blocking findings.

1. Low: `todo.md` names `test_after.log`, but the visible proof artifact in the worktree is `test_before.log`.

   `todo.md:40` says the delegated proof was preserved in `test_after.log`, but no root-level `test_after.log` is present. The existing ignored `test_before.log` contains the same 10-test focused pass described in `todo.md:41` through `todo.md:44`. This does not invalidate the code slice, but the supervisor should normalize canonical proof-log state before treating the next broad-validation result as the acceptance record.

2. Low: The code repair is source-aware and does not overfit the named parser test.

   `parse_base_type()` records whether simple visible type-head resolution came from `VisibleNameSource::UsingAlias` at `src/frontend/parser/impl/types/base.cpp:2326`, then excludes that path from the later source `tag_text_id` restoration at `src/frontend/parser/impl/types/base.cpp:2449`. The branch is keyed to parser lookup metadata rather than a spelling or testcase name, and it leaves the existing typedef preservation path intact for non-using-alias sources.

3. Low: The Step 4 dependent `typename`/cast identity route is preserved.

   The new flag defaults false at `src/frontend/parser/impl/types/base.cpp:1930` and is only set by the simple visible type-head path. The Step 4 route for dependent `typename` and template-member cast targets uses structured owner/deferred-member metadata outside this visible lexical alias case, so the added `!typedef_name_from_using_alias` guard does not demote those identities. The focused proof recorded in `test_before.log` includes `cpp_hir_parser_declarator_deferred_owner_structured_metadata` and the eight generated owner-dependent c-style-cast matrix tests.

## Alignment Judgments

Idea-alignment judgment: matches source idea

Runbook-transcription judgment: plan matches idea

Route-alignment judgment: on track

Technical-debt judgment: acceptable

Validation sufficiency: narrow proof sufficient

Reviewer recommendation: continue current route

## Answer To Review Question

Yes. The visible using-alias TypeSpec metadata repair correctly rejects fabricated typedef `TextId` metadata after scalar typedef expansion while preserving the Step 4 dependent `typename`/cast identity improvements. It does not add a rendered-tag fallback, does not weaken tests, and does not look testcase-shaped. The supervisor can continue Step 6 triage, with the minor caveat that proof-log naming should be normalized before the next broad-validation checkpoint.
