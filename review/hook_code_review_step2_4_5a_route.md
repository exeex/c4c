# Hook Code Review: Step 2.4.4.5A Route

Active source idea path: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Report path: `review/hook_code_review_step2_4_5a_route.md`

Chosen base commit: `1d8408d92^` (`8d4fc0464b...`, parent of `[todo_only] Inventory member typedef mirror consumers`)

Reason: the delegated packet asked for the active-route checkpoint before member-typedef bridge work and named `1d8408d92^` if appropriate. Lifecycle history shows `1d8408d92` is the todo inventory that starts the current member-typedef mirror route, followed by direct/qualified consumer proof, the Step 2.4.4 route split, non-template mirror narrowing, and the Step 2.4.4.5A carrier sequence. The later `024e1cb59 [plan] Retire obsolete member typedef carrier step` is a mid-route plan correction, not a better review base.

Commit count since base: 9

Reviewed head: `9e15e2347895f5bf9ec00706875ac95e02a22c8b`

Working tree note: `todo.md` is currently modified by the hook reminder only. I did not modify it. The uncommitted diff rewrites `Current Step ID` from `Step 2.4.4.5A.3` to stale `Step 2.4.4.3`, rewrites `Current Step Title` to stale `Convert C-Style Cast Type-Id Member-Typedef Consumer`, and adds `你該做code review了`.

## Findings

### High: stale current-step metadata must be repaired before Step 2.4.4.5B execution

The route is ready for Step 2.4.4.5B conceptually, but the current uncommitted hook reminder leaves canonical `todo.md` execution metadata pointing back to `Step 2.4.4.3`. That conflicts with the same file's body, which says Step 2.4.4.5A.3 was just accepted and names Step 2.4.4.5B as the next coherent packet.

Evidence:

- `todo.md:6` currently says `Current Step ID: Step 2.4.4.3`.
- `todo.md:7` currently says `Current Step Title: Convert C-Style Cast Type-Id Member-Typedef Consumer`.
- `todo.md:12` says Step 2.4.4.5A.3 recorded the alias-template carrier review checkpoint.
- `todo.md:24` says the next coherent packet is Step 2.4.4.5B bridge deletion.

Impact: an executor delegated from the current metadata could route itself to an already completed C-style cast packet instead of Step 2.4.4.5B. This is not a source-idea or `plan.md` rewrite problem; a todo-level lifecycle repair is sufficient, but it should happen before more implementation work.

### Low: rendered/deferred compatibility bridge remains live as the intended Step 2.4.4.5B target

The diff has not prematurely deleted the bridge. `apply_alias_template_member_typedef_compat_type`, the dependent rendered/deferred `TypeSpec` projection, and `find_alias_template_info_in_context` fallback are still present and are explicitly named in `todo.md` as Step 2.4.4.5B work. The prior acceptance review correctly treated the structured carrier as progress and the rendered projection as remaining debt.

Evidence:

- `src/frontend/parser/impl/declarations.cpp:257` defines `apply_alias_template_member_typedef_compat_type`.
- `src/frontend/parser/impl/declarations.cpp:265` renders the owner spelling for the compatibility `TypeSpec` projection.
- `src/frontend/parser/impl/declarations.cpp:294` writes `deferred_member_type_name`.
- `src/frontend/parser/impl/types/base.cpp:2131` enters the dependent-argument deferral branch.
- `src/frontend/parser/impl/types/base.cpp:2132` renders owner spelling and `src/frontend/parser/impl/types/base.cpp:2153` writes the deferred member name.
- `src/frontend/parser/impl/types/base.cpp:1674` retains `find_alias_template_info_in_context` as fallback after the structured key lookup.

Impact: this is acceptable technical debt for the route only because Step 2.4.4.5B owns deleting or structurally bypassing it. It must not be counted as already removed.

## Structural Checks

Source idea alignment: passes. The reviewed route removes or narrows parser/Sema rendered-string authority where structured metadata exists, and it does not weaken supported behavior or introduce testcase-shaped shortcuts.

Step 2.4.4.4 non-template mirror narrowing: passes. `c16dd5c06` removes the ordinary non-template `register_structured_typedef_binding(member_key, ...)` publication from `register_record_member_typedef_bindings()` without deleting the dependent/template bridge early.

Step 2.4.4.5A carrier construction: passes. `ParserAliasTemplateMemberTypedefInfo` stores `QualifiedNameKey owner_key`, structured owner args, and `TextId member_text_id` in `src/frontend/parser/parser_types.hpp:154`. The producer calls `try_parse_alias_template_member_typedef_info(parser)` before `parse_type_name()` in `src/frontend/parser/impl/declarations.cpp:1498`, then stores the carrier in active parser state at `src/frontend/parser/impl/declarations.cpp:1541` and copies it into `ParserAliasTemplateInfo` at `src/frontend/parser/impl/declarations.cpp:1982`.

Step 2.4.4.5A consumer route: passes. `resolve_structured_alias_member_type()` in `src/frontend/parser/impl/types/base.cpp:2096` resolves from `ati->member_typedef.owner_key`, parsed carrier args, `TemplateInstantiationKey`, and member `TextId` before the old alias substitution fallback. It does not make rendered owner spelling the normal success route for concrete alias-template member typedef resolution.

Test shape: acceptable. The reviewed window adds focused positive C++ tests for global/namespace member typedef C-style casts and for alias-template carrier disagreement cases. I found no expectation downgrade, unsupported reclassification, or named-test shortcut in the reviewed diff.

Validation artifact: acceptable for the current checkpoint. `test_after.log` exists and ends with 881/881 `cpp_positive_sema_` tests passed; `test_before.log` also exists. The Step 2.4.4.5A review artifact notes the supervisor restored the proof state and regression guard accepted 881/881 before and after.

## Judgments

Idea-alignment judgment: matches source idea.

Runbook-transcription judgment: plan matches idea.

Route-alignment judgment: on track after todo metadata repair.

Technical-debt judgment: watch.

Validation sufficiency: narrow proof sufficient for proceeding to the next narrow packet; Step 2.4.4.5B should still run fresh build plus focused `cpp_positive_sema_` proof after its changes.

Reviewer recommendation: rewrite plan/todo before more execution, limited to repairing `todo.md` current-step metadata/reminder state. Do not rewrite the source idea or `plan.md`. After that repair, continue current route with Step 2.4.4.5B bridge deletion. No testcase-overfit, expectation downgrade, rendered-string rewrapping accepted as completed bridge removal, or premature bridge deletion was found.
