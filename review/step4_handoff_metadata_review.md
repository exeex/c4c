# Step 4 Handoff Metadata Review

Active source idea path: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Chosen base commit: `9ee1c0c13` (`[todo_only] Advance idea 139 lifecycle to Step 4`).

Why this base: it is the lifecycle checkpoint that moved the active idea from Step 3.3 into Step 4, and it is the first commit before the accumulated parser/Sema metadata-handoff implementation now under review. Later lifecycle commit `62ca2bb7b` records the NTTP default blocker but does not reset the Step 4 route, so using it as base would hide earlier Step 4 implementation.

Commit count since base: 6 commits through `HEAD` (`32394adfc`).

Reviewed diff: `git diff 9ee1c0c13..HEAD`.

## Findings

No blocking findings.

The Step 4 implementation still matches idea 139. The recent changes add or preserve structured carriers instead of re-entering semantic lookup through rendered spelling:

- Namespace-qualified free-function declarations now resolve `QualifiedNameRef` to namespace context plus base `TextId` before Sema registration. Relevant code: `src/frontend/parser/impl/declarations.cpp:186`, `src/frontend/parser/impl/declarations.cpp:3427`, `src/frontend/parser/impl/declarations.cpp:3693`.
- Deferred/parser NTTP default lookup now receives `ParserNttpBindingMetadata` and treats structured metadata miss as authoritative before consulting rendered NTTP names. Relevant code: `src/frontend/parser/impl/types/template.cpp:402`, `src/frontend/parser/impl/types/template.cpp:622`, `src/frontend/parser/impl/types/template.cpp:867`.
- Forwarded consteval NTTP arguments now use parser-carried `template_arg_nttp_text_ids` before legacy rendered-name lookup, and metadata miss blocks fallback. Relevant code: `src/frontend/sema/consteval.cpp:247`, `src/frontend/sema/consteval.cpp:576`.
- Qualified consteval lookup now includes namespace/global qualifier metadata in the structured key rather than collapsing to unqualified rendered name. Relevant code: `src/frontend/sema/consteval.cpp:702`, `src/frontend/sema/validate.cpp:941`.

Watch item: the legacy NTTP string maps still exist when no authoritative metadata is present (`src/frontend/sema/consteval.cpp:592` and parser `std::vector<std::pair<std::string, long long>>` binding plumbing). That is not a route failure for this packet because the new behavior blocks rendered fallback after structured metadata is present or misses, and `todo.md` records the missing NTTP default handoff boundary. Keep the next packet scoped to another parser/Sema metadata producer/consumer gap, or route cross-module carrier work to the appropriate metadata idea rather than widening Step 4.

## Testcase-Overfit Check

No testcase-overfit evidence found. The diff does not downgrade supported expectations, mark tests unsupported, add named-case semantic shortcuts, or introduce rendered-string wrapper helpers as claimed cleanup. The new tests mutate rendered declaration/call spelling and NTTP binding names to prove structured metadata wins across the same feature families:

- `tests/frontend/frontend_parser_lookup_authority_tests.cpp:310`
- `tests/frontend/frontend_parser_lookup_authority_tests.cpp:1119`
- `tests/frontend/frontend_parser_lookup_authority_tests.cpp:1200`
- `tests/frontend/frontend_parser_lookup_authority_tests.cpp:1279`

## Proof Review

`test_after.log` is fresh for the delegated command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`

Result in log: build succeeded and the filtered CTest subset passed `885/885`. This is sufficient narrow proof for the accepted Step 4 slices. A broader pass can wait for a Step 4 milestone or larger blast-radius change.

## Judgments

Idea-alignment judgment: `matches source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `on track`

Technical-debt judgment: `watch`

Validation sufficiency: `narrow proof sufficient`

Reviewer recommendation: `continue current route`
