# Idea 133 Step 4 Repair Review

## Review Base

- Chosen base commit: `88203c5f` (`[plan] Activate parser namespace visible-name compatibility cleanup`)
- Reason: this is the lifecycle-tagged activation commit for the currently active idea 133 plan. Later lifecycle commits in this range are `todo.md` step advances, not plan resets or reviewer checkpoints.
- Commits reviewed: `10` commits from `88203c5f..HEAD`
- Focus commits reviewed: `ec08d028`, `5e436f81`, `a4348f2e`

## Findings

No blocking findings.

The Step 4 repair does not reintroduce compatibility spelling as broad semantic authority. The risky repair path is `lookup_type_in_context` in `src/frontend/parser/impl/core.cpp:3142`, where a rendered candidate is still consulted after structured lookup misses. That branch is now constrained to either explicit TextId-less compatibility or a record-only projection via `is_record_projection_type()` in `src/frontend/parser/impl/core.cpp:225`. This is narrower than the rejected broad `qualified_key_in_context(..., create_missing_path=false)` fallback, which remains removed.

The record-projection repair is not testcase-shaped in the diff reviewed. It is framed as a parser record-type projection rule, not a named EASTL/std-vector shortcut, and it preserves the Step 3 demotion for non-record rendered typedefs. The white-box test at `tests/frontend/frontend_parser_tests.cpp:1765` verifies that valid-TextId rendered-only non-record type/value lookups are not promoted, while TextId-less compatibility remains explicit.

The active-record additions in `resolve_visible_type` at `src/frontend/parser/impl/core.cpp:2422` are narrow enough for the repaired failures: current self-type spelling and same-namespace incomplete sibling record projection are gated to C++ record-body context. This is still compatibility-spelling-sensitive, so it should remain a Step 5 validation focus, but it is not broad enough to block advancement.

The using-value alias route remains aligned. `lookup_using_value_alias` now requires structured targets to resolve through known function or structured var bindings before succeeding (`src/frontend/parser/impl/core.cpp:2908`), and only no-key aliases keep explicit compatibility bridge behavior. That matches the source idea’s requirement that `compatibility_name` not validate a missing structured target.

The string bridge comments added in `src/frontend/parser/parser.hpp:517` and nearby `core.cpp` definitions accurately classify string overloads as compatibility entry points or projection bridges. They do not by themselves enforce authority, but the reviewed call-site changes in `src/frontend/parser/impl/types/base.cpp` move the relevant parser-owned type/value probes to TextId-aware overloads.

## Plan Alignment

- Judgment: `on track`
- Rationale: Step 4 asks to quarantine string overloads and AST projection bridges, convert semantic call sites to structured carriers, and keep remaining string-returning helpers only as projection or explicit bridge boundaries. The current implementation does that for the touched parser namespace/type/value lookup families.

## Idea Alignment

- Judgment: `matches source idea`
- Rationale: the implementation keeps compatibility spelling as display/projection or explicit fallback, while preserving structured-primary lookup through `QualifiedNameKey`, `TextId`, and namespace context ids. The remaining record projection is a compatibility bridge, but it is visible and constrained rather than a silent general lookup authority.

## Technical Debt

- Judgment: `watch`
- Rationale: record projection still lives inside semantic type lookup, not only at AST projection boundaries. That appears necessary for current parser record/tag representation, but Step 5 should keep it under validation pressure and avoid expanding it to non-record or arbitrary rendered typedef lookup.

## Validation Sufficiency

- Judgment: `needs broader proof`
- Rationale: the Step 4 proof is sufficient to advance to Step 5 validation, but not sufficient as final acceptance. It includes `frontend_parser_tests` plus the rejected-baseline repair cases and passes in `test_after.log`. Because `lookup_type_in_context` and `resolve_visible_type` are broad parser lookup surfaces, Step 5 should run the planned targeted parse-only selectors for using aliases, namespace directives, qualified namespace lookup, and visible-name lookup, and consider broader parser CTest if the supervisor treats this as the final acceptance gate.

## Recommendation

- Reviewer recommendation: `continue current route`
- Next action: advance to Step 5 validation with the record-projection bridge called out as a watch item, not a blocker.
