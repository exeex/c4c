# Step 3 EASTL Member Typedef Repair Review

Active source idea path: `ideas/open/146_qualified_name_deferred_carrier_authority.md`

Chosen base commit: `7c4e58b76` (`[plan] Activate qualified name deferred carrier plan`). This commit activated the current source idea into `plan.md`/`todo.md`; later `e60d19d49` is a todo-only inventory checkpoint, not a route reset or source-intent change. The current dirty Step 3 repair is therefore reviewed as part of the active idea route from activation through `HEAD` plus the working-tree diff.

Commit count since base: 7 commits from `7c4e58b76..HEAD`, plus the current uncommitted Step 3 repair diff.

## Findings

### Medium: New test proves the regression but not a collision/stale-authority case

- `tests/frontend/frontend_parser_lookup_authority_tests.cpp:5128` adds a focused EASTL-shaped partial-specialization member typedef test, but the primary and partial specialization both define `no_type_helper` and `typedef no_type_helper type`.
- `tests/frontend/frontend_parser_lookup_authority_tests.cpp:5188` only requires that `parse_base_type()` resolves to some complete `TB_STRUCT` record, not that it selected the partial-specialization owner rather than a stale/rendered or primary-family route.
- This is not a blocking overfit signal because the implementation itself uses `QualifiedNameRef`, `QualifiedNameKey`, concrete template argument keys, and structured member typedef lookup. However, the added test is weaker than the idea acceptance criterion that asks for a stale-route/collision case where rendered splitting would be wrong.

### Low: Structured route is aligned, but the helper fallback is broad enough to watch

- `src/frontend/parser/impl/types/base.cpp:1464` completes member typedef `record_def` from owner fields by matching `TextId` metadata, which is aligned with structured carrier authority.
- `src/frontend/parser/impl/types/base.cpp:1491` and `src/frontend/parser/impl/types/base.cpp:1502` also allow a single anonymous nested record to complete an otherwise carrierless member type. That fallback does not split rendered qualified spellings, but it can mask missing `TextId` carrier metadata and should stay bounded to this repair route.
- The same completion logic is duplicated around `src/frontend/parser/impl/types/base.cpp:4134` and `src/frontend/parser/impl/types/base.cpp:7905`, creating maintenance debt rather than immediate route drift.

### Low: Qualified template owner handling remains structured, not rendered-string authority

- `src/frontend/parser/impl/types/base.cpp:2466` starts from `peek_qualified_name()` and `src/frontend/parser/impl/types/base.cpp:2541` stores `qualified_name_key(qn)` on the owner carrier.
- `src/frontend/parser/impl/types/base.cpp:2595` instantiates from parsed template arguments, and `src/frontend/parser/impl/types/base.cpp:2620` resolves the member typedef through `lookup_struct_member_typedef_recursive_for_type()`.
- The rendered `qualified_name` produced at `src/frontend/parser/impl/types/base.cpp:2486` is used as origin/display fallback and not split back into semantic components in this slice. The `struct_tag_def_map` lookup at `src/frontend/parser/impl/types/base.cpp:2606` is a mangled-instantiation fallback after structured primary lookup/argument parsing, not the primary authority path.

## Judgments

- Idea alignment: matches source idea.
- Runbook transcription: plan matches idea.
- Route alignment: on track.
- Technical debt: watch.
- Validation sufficiency: narrow proof sufficient for this Step 3 repair slice, with the caveat that the next acceptance/milestone should add or preserve a stronger collision/stale-route test.

## Proof Assessment

`test_after.log` shows `cmake --build --preset default` and the delegated five-test subset passed, including `frontend_parser_lookup_authority_tests`, `frontend_parser_tests`, namespace qualified-name tests, and the EASTL specialization parse case. That is sufficient for accepting this focused parser repair because the touched code is parser/type-member typedef logic and the subset includes both authority coverage and the target regression.

## Recommendation

Reviewer recommendation: continue current route.

Supervisor may accept this Step 3 repair if comfortable carrying the test-strength caveat as follow-up. I do not see testcase-shaped semantic shortcuts, expectation weakening, unsupported reclassification, or restored rendered compound `TextId` semantic authority in the current diff.
