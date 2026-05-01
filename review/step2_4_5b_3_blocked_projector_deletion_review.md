# Step 2.4.4.5B.3 Blocked Projector Deletion Review

Active source idea path: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Report path: `review/step2_4_5b_3_blocked_projector_deletion_review.md`

Chosen base commit: `6b39307ca` (`Record reviewed record-scope sidecar route`)

Reason: this is the last reviewer-checkpoint commit for the active idea and Step 2.4.4.5B route. It records `review/step2_4_5b_1_failed_record_scope_carrier_review.md`, advances `todo.md` to the reviewed B.2 sidecar packet, and is immediately before the implementation commit `0389c6abc` and blocked deletion record `a5620eb6e`. Reviewing from this base covers the current route question without re-litigating the earlier reverted carrier attempt.

Commit count since base: 2

Working tree: clean before review.

## Findings

### High: the next carrier still fits Step 2.4.4.5B; do not rewrite `plan.md` before implementation

The blocked deletion did what Step 2.4.4.5B.3 requires when another missing carrier appears: it recorded the exact remaining blocker in `todo.md` before proceeding. The new blocker is not a separate initiative. It is still the same parser/Sema member-typedef bridge removal route: delete `apply_alias_template_member_typedef_compat_type` by replacing its semantic effect with structured owner/member metadata.

Evidence:

- `plan.md:660` through `plan.md:694` define Step 2.4.4.5B as replacing the dependent/template member-typedef bridge and routing alias member-typedef readers through structured metadata.
- `plan.md:778` through `plan.md:795` explicitly allow B.3 to record another exact missing carrier if deletion exposes one.
- `todo.md:24` through `todo.md:33` identifies the exact remaining carrier as top-level template using-alias RHS propagation for `typename Owner<Args>::member`.
- `src/frontend/parser/impl/declarations.cpp:257` through `src/frontend/parser/impl/declarations.cpp:297` show the retained projector is still the bridge to delete.

The next implementation packet should therefore be a narrowed Step 2.4.4.5B carrier repair, not a route reset. The route-quality action is: add the missing top-level template using-alias RHS structured carrier needed to make `typename Owner<Args>::member` resolvable without fabricating a deferred `TypeSpec`, then retry deleting `apply_alias_template_member_typedef_compat_type`.

### Medium: `todo.md` needs metadata repair before an executor packet; `plan.md` does not

The execution metadata is inconsistent. `todo.md` says `Current Step ID: Step 2.4.4.5B.3`, while `scripts/plan_review_state.py show` reports `Step 2.4.4.5B.2`. Also, B.3 is titled as projector deletion, but `todo.md`'s suggested next work is a carrier implementation packet. That mismatch can misroute the next executor even though the runbook itself remains usable.

Evidence:

- `todo.md:7` and `todo.md:8` name the current step as B.3 projector deletion.
- `scripts/plan_review_state.py show` reports `current_step_id` as `Step 2.4.4.5B.2` and title `Implement Reviewed Record-Scope Using-Alias RHS Sidecar Carrier`.
- `todo.md:24` through `todo.md:27` says the next packet should add a missing structured carrier, not delete the projector directly.

Recommended repair is todo/state-only: align the current packet metadata to the narrowed carrier follow-up before implementation. Do not rewrite the source idea. Do not rewrite `plan.md` unless the plan owner wants an explicit B.4 substep for bookkeeping; the existing B.2/B.3 text already covers "adjacent carrier" and "another missing carrier" enough for execution.

### Medium: the committed record-scope sidecar is structured progress, but it is not sufficient

Commit `0389c6abc` added the record-scope sidecar selected by the prior review. The carrier is captured before `parse_type_name()`, stored as `ParserAliasTemplateMemberTypedefInfo`, persisted by `Record::alias` key, and read before falling back to the flattened member typedef `TypeSpec`.

Evidence:

- `src/frontend/parser/impl/types/struct.cpp:750` through `src/frontend/parser/impl/types/struct.cpp:789` capture RHS owner key, owner args, and member `TextId` before the normal RHS type parse.
- `src/frontend/parser/impl/types/struct.cpp:823` through `src/frontend/parser/impl/types/struct.cpp:832` attach that sidecar to record-scope `using` member typedef storage.
- `src/frontend/parser/impl/types/struct.cpp:2446` through `src/frontend/parser/impl/types/struct.cpp:2450` persist valid sidecars by record member key.
- `src/frontend/parser/impl/types/base.cpp:1063` through `src/frontend/parser/impl/types/base.cpp:1145` resolve the sidecar through structured owner key, substitutable owner args, and member `TextId`.

This is not named-test overfit and does not use the forbidden rendered/debug parsing route. The failed B.3 deletion proves only that one more carrier boundary remains.

### High: do not unblock by restoring the projector's rendered/deferred `TypeSpec` behavior

The retained projector is exactly the compatibility path Step 2.4.4.5B is trying to delete. It renders the owner name, fills `tpl_struct_origin`, populates `debug_text`, and sets `deferred_member_type_name`. Any next patch that restores those effects under a different helper name would be route failure.

Evidence:

- `src/frontend/parser/impl/declarations.cpp:265` through `src/frontend/parser/impl/declarations.cpp:269` render owner/member spelling.
- `src/frontend/parser/impl/declarations.cpp:276` through `src/frontend/parser/impl/declarations.cpp:278` fabricate the owner `TypeSpec`/`tpl_struct_origin`.
- `src/frontend/parser/impl/declarations.cpp:291` assigns template argument `debug_text`.
- `src/frontend/parser/impl/declarations.cpp:294` through `src/frontend/parser/impl/declarations.cpp:296` restores deferred member name fields.

The next packet must preserve `ParserAliasTemplateMemberTypedefInfo` or an equivalent structured carrier directly. Rendered/deferred `TypeSpec` restoration, `debug_text` parsing, split `Owner::member`, and named-fixture shortcuts remain blocking overfit.

## Answer To Review Question

The next packet fits the current Step 2.4.4.5B runbook as a narrowed top-level template using-alias RHS structured-carrier repair. The route is not mismatched enough to rewrite `plan.md` or the source idea before implementation continues.

However, the execution state should be repaired first at the `todo.md`/plan-review-state layer. The next packet should not run under ambiguous "B.3 delete projector" metadata while it is actually adding the carrier that B.3 discovered was missing.

## Judgments

Idea-alignment judgment: matches source idea.

Runbook-transcription judgment: plan is lossy but usable.

Route-alignment judgment: narrow next packet.

Technical-debt judgment: watch; action needed only for the stale execution metadata and retained projector.

Validation sufficiency: needs broader proof after implementation. Use a fresh build plus `ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_'`, and explicitly confirm the two timeout fixtures pass with `apply_alias_template_member_typedef_compat_type` deleted.

Reviewer recommendation: narrow next packet. Do a todo/state-only repair to align the current packet with the top-level template using-alias RHS structured-carrier follow-up, then implement that carrier and retry projector deletion. Do not rewrite `plan.md` unless the plan owner wants to add an explicit bookkeeping substep; do not continue by restoring rendered/deferred TypeSpec behavior or any named-test shortcut.
