# Idea 25 RISC-V Stack Source Edge Publication Review

## Scope

- Active source idea: `ideas/open/25_riscv_prepared_edge_publication_stack_source_register_consumer.md`
- Active plan: `plan.md`
- Current todo: `todo.md`
- Reviewed slice: commit `5ddb6c32f` plus current uncommitted `todo.md` metadata repair that advances Step 3 to Step 4.
- Review focus: RISC-V prepared edge-publication helper and focused tests for `StackSlot -> Register` register-destination moves.

## Review Base

Chosen base: `c46464f0c [plan] Activate RISC-V stack source edge publication plan`.

Reason: this commit creates the current active `plan.md` and `todo.md` for idea 25. The next lifecycle commit, `c865ce828`, is the inventory note for the same route, not a new checkpoint or source-idea rewrite. The active source idea is unchanged after activation.

Commits since base: 2 (`c865ce828`, `5ddb6c32f`). The current `todo.md` metadata repair is uncommitted and limited to `Current Step ID: 4` / `Current Step Title: Validate the RISC-V Stack Source Slice`.

## Findings

No blocking findings.

## Alignment

The slice matches the source idea. `render_edge_publication_source_operand` now accepts only `PreparedValueHomeKind::StackSlot` sources with a concrete offset and 4-byte size, records stack provenance on the intent, and renders the target-local source operand as `<offset>(sp)` in `src/backend/mir/riscv/codegen/emit.cpp:74`. The shared lookup remains the semantic authority: `consume_edge_publication_move_intent` still obtains the publication through `find_unique_indexed_prepared_edge_publication` before rendering source or destination homes in `src/backend/mir/riscv/codegen/emit.cpp:100`.

The implementation does not introduce predecessor/successor scans, local edge-copy rediscovery, fixture-label dispatch, or value-id shortcuts. The emitted instruction is selected from the source-home class after shared lookup authority succeeds: immediates still use `li`, stack sources use `lw`, and register sources use `mv` in `src/backend/mir/riscv/codegen/emit.cpp:142`.

Unsupported homes remain fail closed. Destination homes must still be registers in `src/backend/mir/riscv/codegen/emit.cpp:133`, pointer-base sources are not rendered by the source helper, and malformed stack homes return `UnsupportedSourceHome`.

## Test Coverage

The positive stack-source test mutates the existing prepared edge-publication fixture to a `StackSlot` source with slot id, offset, and size, then proves emitted assembly and the helper intent: `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:205`.

Authority coverage is present: the test clears `publications_by_edge_destination` and expects `MissingPublication`, proving the helper does not rediscover edge moves without the shared lookup index in `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:251`.

Fail-closed coverage was extended for stack sources without size, without offset, and with non-I32 width in `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:281`. Existing pointer-base source, immediate-without-value, stack destination, and non-move-publication negatives remain in the same test body.

## Validation

Recorded proof is sufficient to proceed to Step 5 handoff/closure:

- Focused proof command recorded in `todo.md` passed 5/5 selected tests.
- Matching focused regression guard is recorded as passing with 5/5 before and 5/5 after.
- Broader backend validation is recorded as passing 163/163 backend tests in `test_after.log`.

The backend run is broader validation, not the matching before/after comparison, which is accurately stated in `todo.md`. That is sufficient for this bounded backend helper/test slice.

## Judgments

- Idea alignment: matches source idea.
- Runbook transcription: plan matches idea.
- Route alignment: on track.
- Technical debt: acceptable.
- Validation sufficiency: narrow proof plus backend bucket sufficient.
- Reviewer recommendation: continue current route.

## Handoff Notes

Step 5 may proceed. The handoff should state support exactly as focused `StackSlot -> Register` register-destination edge-publication moves with concrete 4-byte stack-source homes, emitted as `lw <dst>, <offset>(sp)` through the shared `edge_publications` lookup path.

Do not overclaim support for pointer-base sources, non-I32 stack-source widths, missing stack offsets, dynamic stack addressing, large-offset policy, or source-to-`StackSlot` destinations. Those remain unsupported and fail closed.
