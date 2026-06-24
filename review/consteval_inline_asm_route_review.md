# Consteval Inline Asm Route Review

Active source idea path: `ideas/open/343_rv64_consteval_inline_asm_template_strings.md`

Chosen base commit: `b04912e8a [plan+idea] Activate RV64 consteval inline asm template strings`

Why this base: this is the commit that created and activated the current source idea, `plan.md`, and `todo.md` for the consteval inline asm template-string child. Later commits are one mapping-only `todo.md` checkpoint and two implementation/test commits; none is a reviewer checkpoint or source-idea reset.

Commit count since base: 3

## Findings

1. Severity: high - Step 4 cannot be used as closure yet because the accepted compile-time string surface has not been implemented.

   The source idea requires accepted compile-time string expressions, compile-time `+` concatenation, and one constexpr/consteval/helper style when it folds before inline asm lowering (`ideas/open/343_rv64_consteval_inline_asm_template_strings.md:35`, `ideas/open/343_rv64_consteval_inline_asm_template_strings.md:39`, `ideas/open/343_rv64_consteval_inline_asm_template_strings.md:41`, `ideas/open/343_rv64_consteval_inline_asm_template_strings.md:102`). The committed implementation only preserves raw or adjacent string literal handling and rejects non-literal template expressions in `parse_stmt` (`src/frontend/parser/impl/statements.cpp:982`, `src/frontend/parser/impl/statements.cpp:985`, `src/frontend/parser/impl/statements.cpp:1015`). The new object test uses C++ adjacent literal concatenation inside the C++ test source, not a frontend compile-time `+` expression or consteval helper flowing through `asm volatile(...)` (`tests/backend/mir/backend_riscv_object_emission_test.cpp:2049`). This is acceptable preparatory coverage, but it is not enough to enter the current `plan.md` Step 4 "Broaden Proof And Prepare Closure" path (`plan.md:160`).

2. Severity: medium - The current `todo.md` metadata is stale and conflicts with its body.

   `todo.md` says `Current Step ID: 2` and `Current Step Title: Define And Test The Positional .insn.d Shape` (`todo.md:4`), but the packet body says Step 3 just finished and suggests Step 4 next (`todo.md:12`, `todo.md:26`). The hook-added review reminder is valid context, but the metadata should be realigned before delegation so the next executor does not receive a Step 2/Step 4 mixed signal.

3. Severity: low - The parser diagnostic gate and negative harness change are aligned and useful.

   Runtime/non-literal asm template expressions now fail with a specific diagnostic (`src/frontend/parser/impl/statements.cpp:950`, `src/frontend/parser/impl/statements.cpp:1015`), and the negative harness checks that exact substring for `bad_inline_asm_runtime_template` (`tests/c/internal/cmake/run_negative_case.cmake:30`, `tests/cpp/internal/negative_case/CMakeLists.txt:10`, `tests/cpp/internal/negative_case/bad_inline_asm_runtime_template.cpp:4`). This supports the source idea's rejection criteria (`ideas/open/343_rv64_consteval_inline_asm_template_strings.md:106`) and does not weaken existing expectations.

4. Severity: low - The adjacent `.insn.d` object coverage is acceptable as a downstream equivalence guard, but should not be counted as helper support.

   The new object test confirms the existing `.insn.d` object path still emits the expected `0x0000030b10620a0a` bytes when the final template text is assembled from adjacent literal fragments (`tests/backend/mir/backend_riscv_object_emission_test.cpp:2049`). That is a useful guard for downstream object behavior, not a proof of the source idea's compile-time string-expression or consteval helper acceptance criteria.

## Judgments

Idea-alignment judgment: `matches source idea`

Runbook-transcription judgment: `plan is lossy but usable`

Route-alignment judgment: `drifting`

Technical-debt judgment: `watch`

Validation sufficiency: `needs broader proof`

Reviewer recommendation: `rewrite plan/todo before more execution`

## Recommendation Details

Do not close or broaden for acceptance from the current Step 4 wording. Ask the plan owner to rewrite `todo.md` and, if needed, the Step 4 portion of `plan.md` so the next packet explicitly implements or proves the narrow first real compile-time string surface before closure. The source idea does not need rewriting: its intent and reject signals are still the right contract.

The next executable packet should narrow to one of these outcomes:

- implement a real accepted compile-time template expression path, such as string-literal `+` folding or an existing fixed-string representation, before inline asm lowering; or
- if no existing representation can carry that surface without a larger frontend change, record the blocker in `todo.md` and route a plan-owner split instead of treating adjacent literals as the final feature.

No testcase-overfit finding: the recent changes do not add a named EV shortcut, do not bypass the existing inline asm/object route, and do not downgrade tests. The issue is proof and lifecycle sequencing, not an overfit implementation.
