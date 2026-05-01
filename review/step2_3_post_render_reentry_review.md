# Step 2.3 Post Render Re-entry Review

Active source idea: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Chosen base commit: `6b9da6879` (`[plan+idea] Route remaining fallback cleanup boundaries`)

Why this base: it is the lifecycle commit that materially reset the active idea/runbook boundary for the remaining parser fallback cleanup. `9de313e1e` then aligned `todo.md` for Step 2.3, but did not change source intent. Reviewing `6b9da6879..HEAD` covers the full active Step 2.3 implementation line, including the requested commits `bd7e4435d`, `9c2ce7068`, `d3c0ee025`, and `cc842582b`.

Commit count since base: 11 commits.

## Findings

1. Medium: validation proof is not acceptance-ready because the canonical after log named by `todo.md` is missing.

   `todo.md` currently records proof as:

   ```text
   (... frontend_parser_lookup_authority_tests ...) > test_after.log 2>&1
   Proof log: test_after.log
   ```

   But the workspace has no `test_after.log`; only `test_before.log`, `test_baseline.log`, and `test_baseline.new.log` are present. `test_before.log` contains a passing `frontend_parser_lookup_authority_tests` run, so the focused target appears to have been executed, but the canonical proof artifact is mislabeled/misplaced for executor acceptance. This is a process blocker, not an implementation blocker. Regenerate `test_after.log` with the delegated command before accepting or committing the slice.

2. Low: Step 2.3 now appears exhausted and should be handed to plan-owner unless the supervisor has a named remaining parser/Sema rendered-authority route.

   The recent slices removed concrete Step 2.3 parser re-entry points:

   - `src/frontend/parser/impl/core.cpp:2662` rejects no-key using-value alias bridges and keeps alias success keyed by `QualifiedNameKey`.
   - `src/frontend/parser/impl/core.cpp:2693` and `src/frontend/parser/impl/core.cpp:2723` make known-function lookup success key-authoritative rather than dependent on rendered display production.
   - `src/frontend/parser/impl/core.cpp:1312` removes alias-template recovery through `resolve_visible_type(...) -> visible_name_spelling(...) -> find_parser_text_id(...)`.
   - `src/frontend/parser/impl/types/types_helpers.hpp:619` trusts a structured qualified type result instead of requiring a rendered typedef `TextId` re-entry.
   - `src/frontend/parser/impl/types/declarator.cpp:726` removes the dependent-typename visible-type spelling bridge before typedef success.

   Remaining watchouts in `todo.md` are mostly explicit blockers or out-of-scope compatibility paths, such as `find_var_type(TextId)` retaining legacy symbol-table compatibility at `src/frontend/parser/impl/core.cpp:1053`. That retained path is not presented as completed Step 2.3 progress and should not be silently expanded in this step. If the supervisor wants to keep attacking it, plan-owner should split a new concrete metadata packet or route it to the appropriate follow-up idea.

## Alignment

Idea-alignment judgment: matches source idea.

The implementation deletes rendered-string semantic authority instead of renaming it. The new tests exercise drifted rendered spellings and legacy rendered storage losing to structured keys, especially `tests/frontend/frontend_parser_lookup_authority_tests.cpp:76`, `tests/frontend/frontend_parser_lookup_authority_tests.cpp:114`, `tests/frontend/frontend_parser_lookup_authority_tests.cpp:146`, and `tests/frontend/frontend_parser_lookup_authority_tests.cpp:187`.

Runbook-transcription judgment: plan matches idea.

Step 2.3 names parser declarator, value, known-function, qualified declarator names, and visible type lookup. The reviewed commits stay inside that parser/Sema-rendered-lookup-removal surface and do not pull HIR/LIR/backend cleanup into idea 139.

Route-alignment judgment: on track.

No testcase-overfit or helper-only rename pattern found. The tests are white-box but semantic: they seed structured and rendered authorities into disagreement and assert that lookup follows the structured carrier or rejects the legacy bridge. They are not narrowing production behavior to one named source testcase, and no supported-path expectations are downgraded.

Technical-debt judgment: watch.

The compatibility names on `VisibleNameResult` remain as display payload, and several broader parser/Sema string surfaces still exist. The current `todo.md` classifies the important residuals instead of claiming they are complete. That is acceptable for this Step 2.3 slice, but the step should not continue by opportunistically chasing unrelated `fallback`/`legacy` grep hits without plan-owner splitting.

Validation sufficiency: needs broader proof.

The focused target is the right narrow target for parser lookup authority, but current canonical proof state is insufficient because `test_after.log` is absent. Given the touched files include parser core/declarations, parser tests, and CMake registration, I recommend at least regenerating the focused `test_after.log`; for acceptance, prefer the internal parse pair:

```sh
cmake --build build --target frontend_parser_tests frontend_parser_lookup_authority_tests &&
ctest --test-dir build -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$' --output-on-failure
```

If `frontend_parser_tests` remains blocked by the qualified `TypeSpec` metadata expectation already recorded in `todo.md`, the supervisor should decide whether that is a separate known baseline before accepting Step 2.3 on the focused target alone.

Reviewer recommendation: rewrite plan/todo before more execution.

This does not mean reset the route. It means route Step 2.3 exhaustion through plan-owner: either advance/close this parser substep after proof repair, or split a specifically named remaining metadata blocker. Continuing Step 2.3 with another grep-driven parser/Sema string cleanup packet would risk drift from idea 139's "remove concrete semantic authority routes" rule.
