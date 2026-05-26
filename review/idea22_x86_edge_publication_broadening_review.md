# Idea 22 x86 Edge Publication Broadening Review

## Scope

- Active source idea: `ideas/open/22_x86_prepared_edge_publication_coverage_broadening.md`
- Active plan: `plan.md`
- Current lifecycle context: `todo.md`
- Review range: `9180df21f..HEAD`
- Chosen base commit: `9180df21f` (`[plan] Activate x86 edge publication coverage plan`)
- Commit count since base: 2

`9180df21f` is the correct active-idea checkpoint because it creates the current `plan.md` and `todo.md` for `ideas/open/22_x86_prepared_edge_publication_coverage_broadening.md`. The next lifecycle commit, `d240c4238`, maps the target within that active plan; `723d10be1` implements the selected register-source broadening.

Note: the working tree has an unstaged `todo.md` change advancing metadata from `Current Step ID: 2/3` to `Current Step ID: 4` and adding the review reminder line. The implementation review below is for `9180df21f..HEAD`; the unstaged `todo.md` state is considered current lifecycle context.

## Findings

### Medium: canonical proof log referenced by `todo.md` is missing from the workspace

- `todo.md:47` records a passed build plus focused CTest command.
- `todo.md:50` names `test_after.log` as the proof log.
- Current workspace inspection found `test_before.log` present, but no `test_after.log`.

This does not imply the implementation is wrong, but it makes the recorded proof non-auditable from the current workspace. The command in `todo.md` is an appropriate focused backend subset for the touched x86 prepared surface, but acceptance should regenerate or restore the canonical `test_after.log` before treating the slice as validation-ready.

## Alignment Notes

The implementation broadens the existing x86 prepared edge-publication consumer instead of adding a new local semantic source. `consume_edge_publication_move_intent` still starts from `ConsumedPlans::shared_function_lookups()->edge_publications` and `find_unique_indexed_prepared_edge_publication` before classifying or rendering any move intent (`src/backend/mir/x86/prepared/dispatch.cpp:58`). The added helper only renders target-owned x86 source operands for homes already attached to the shared publication (`src/backend/mir/x86/prepared/dispatch.cpp:10`).

The new accepted shape is a home-family broadening, not a named-case shortcut: register source homes with register names now join the existing stack-slot source support, while missing source homes and unsupported source homes still return explicit unsupported statuses (`src/backend/mir/x86/prepared/dispatch.cpp:88`, `src/backend/mir/x86/prepared/dispatch.cpp:101`). Destination homes remain restricted to register destinations, so memory-destination policy and scratch/clobber decisions are not silently invented (`src/backend/mir/x86/prepared/dispatch.cpp:92`).

The focused tests cover both direct helper behavior and module-route behavior. Direct coverage mutates the fixture source home to `PreparedValueHomeKind::Register`, verifies the returned shared publication pointer, verifies `mov ebx, eax`, and proves no emission occurs without shared lookups (`tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp:650`). The module-route test now forces two register source homes and verifies two emitted shared-publication moves, then mutates the shared publication destination and expects rejection rather than fallback rediscovery (`tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp:7711`, `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp:7762`).

No test expectation was downgraded to unsupported, and the existing direct stack-slot-to-register helper/emitter coverage remains present (`tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp:773`). The module-route test changed from stack-source emission to register-source emission, but that is consistent with the selected broadening target and is not the main proof of stack-source preservation.

## Judgments

- Idea-alignment judgment: `matches source idea`
- Runbook-transcription judgment: `plan matches idea`
- Route-alignment judgment: `on track`
- Technical-debt judgment: `acceptable`
- Validation sufficiency: `needs broader proof`
- Reviewer recommendation: `continue current route`

The route should continue, with the next supervisor action focused on Step 4 validation. Before acceptance or closure, regenerate or otherwise make available the canonical `test_after.log` matching the command recorded in `todo.md`, then decide whether a broader regression guard is needed for closure.
