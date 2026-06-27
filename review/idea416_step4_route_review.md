# Idea 416 Step 4 Route Review

Active source idea path: `ideas/open/416_prepared_helper_operand_home_contracts.md`

Chosen review base: `31cf8da95` (`[plan] activate prepared helper operand home contracts plan`).

Why this base: this commit activates the current idea-416 runbook and creates the matching `todo.md` for `ideas/open/416_prepared_helper_operand_home_contracts.md`. Later commits are the route execution commits for the same active source idea rather than a new lifecycle checkpoint.

Commit count since base: 7.

## Findings

1. Medium: `todo.md` metadata is inconsistent with the actual execution state and should be repaired before another executor packet.

   `todo.md:6` and `todo.md:7` currently say `Current Step ID: Step 3` / `Current Step Title: Introduce Typed Per-Helper Payloads`, but the same file records that Step 4 was just finished at `todo.md:12` and suggests continuing Step 4 at `todo.md:24`. The hook-backed review state also mirrors Step 3 while `code_review_pending=true`, so the next packet could be shaped from stale metadata. This does not require a source-idea or `plan.md` rewrite; a `todo.md` lifecycle/metadata repair is enough.

2. Low: validation evidence is narrow and the canonical after-log is not currently present in the worktree.

   `todo.md:49` records the Step 4 proof command and `todo.md:52` records a pass, but `test_after.log` is absent while `test_before.log` and `test_baseline.log` remain at repo root. For route review this is not a code blocker because the committed tests cover the intended helper contract behavior, but acceptance should regenerate canonical proof before committing the next code slice or treating Step 4 as complete.

## Alignment Review

The implementation route still matches the source idea. The durable idea asks for typed per-helper variadic operand contracts, producer-owned fail-closed diagnostics, and no target-side inference of helper homes. The committed code follows that route:

- `docs/prepared_fact_contracts/helper_operand_home_contract_plan.md:17` through `docs/prepared_fact_contracts/helper_operand_home_contract_plan.md:28` cite the intended taxonomy rows and owner classes.
- `src/backend/prealloc/variadic.hpp:306` through `src/backend/prealloc/variadic.hpp:345` introduce typed `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy` payloads while keeping the legacy optional fields as compatibility storage.
- `src/backend/prealloc/variadic.hpp:468` through `src/backend/prealloc/variadic.hpp:681` expose typed accessors/publishers that reject wrong-helper and incomplete payloads and clear stale cached payloads when current optional facts are missing.
- `src/backend/prealloc/prepared_contract_verifier.cpp:408` through `src/backend/prealloc/prepared_contract_verifier.cpp:423` routes helper coherence through the typed accessors, and `src/backend/prealloc/prepared_contract_verifier.cpp:780` through `src/backend/prealloc/prepared_contract_verifier.cpp:803` preserves missing vs incoherent owner classification.
- `src/backend/prealloc/variadic_entry_plans.cpp:1071`, `src/backend/prealloc/variadic_entry_plans.cpp:1092`, `src/backend/prealloc/variadic_entry_plans.cpp:1101`, and `src/backend/prealloc/variadic_entry_plans.cpp:1123` publish AArch64 typed helper payloads at the producer boundary; `src/backend/prealloc/variadic_entry_plans.cpp:1183`, `src/backend/prealloc/variadic_entry_plans.cpp:1216`, `src/backend/prealloc/variadic_entry_plans.cpp:1226`, and `src/backend/prealloc/variadic_entry_plans.cpp:1276` do the same for RV64.

The Step 4 accessor/verifier direction is acceptable. The compatibility behavior that complete legacy optional rows can derive or refresh typed payloads is a reasonable transitional mechanism for Step 3/4 because the target consumers are not migrated yet. The important guard is present: stale cached payloads do not mask missing current optional facts, and wrong-helper/incomplete rows fail closed. The focused verifier tests cover this shape for all four helper families at `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp:522`, `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp:653`, `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp:830`, and `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp:960`. The scalar refresh repair is specifically covered at `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp:701` through `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp:823`.

No testcase-overfit pattern was found. I did not see expectation downgrades, allowlist filtering, named torture-case shortcuts, target-side address synthesis, or printed-text backend shape matching in the reviewed range.

## Judgments

Idea-alignment judgment: `matches source idea`.

Runbook-transcription judgment: `plan matches idea`.

Route-alignment judgment: `on track`.

Technical-debt judgment: `watch`.

Validation sufficiency: `needs broader proof`.

Reviewer recommendation: `rewrite plan/todo before more execution`.

## Recommended Next Action

Repair only `todo.md` metadata before more execution: restore the current step fields to Step 4, keep the hook-triggered review reminder only as needed, and record this review result. Do not rewrite the source idea or `plan.md`.

After that, continue the current route with a narrowed Step 4 packet: review remaining verifier/report call sites that still depend on generic optional-bag completeness for variadic helper operand homes, then regenerate canonical proof logs. Target-consumer migration should wait until that Step 4 cleanup is done.
