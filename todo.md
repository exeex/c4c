Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Prepared Placement Types

# Current Packet

## Just Finished

Step 2 added the first structured prepared placement identity surface for idea 223 without migrating all legacy consumers. `PreparedRegisterPlacement` now carries bank, slot pool, slot index, and contiguous width; `PreparedSpillSlotPlacement` carries spill frame-slot identity. Target-register profile helpers now publish structured candidate-span placement and ABI arg/result placement helpers while keeping legacy spelling helpers intact.

Representative prepared publications now carry structured placement alongside compatibility spelling fields: register candidate spans, assigned/spill-authority registers, assigned stack slots, frame saved registers, call clobbers as reserved scratch, ABI argument/result call plans, storage-plan register homes, and spill/reload frame-slot placement. Focused prepared tests now assert abstract placement for argument, return, caller-temp/callee spans, reserved scratch, long-lived callee assignment, and spill placement.

## Suggested Next

Next coherent packet: migrate one narrow prepared consumer family to inspect `PreparedRegisterPlacement`/`PreparedSpillSlotPlacement` first, leaving legacy spelling fields as compatibility display or fallback. Keep this limited to pre-target prepared surfaces unless the supervisor explicitly opens an AArch64 handoff packet.

## Watchouts

- This packet intentionally did not rewrite `regalloc.cpp` or downstream AArch64 consumers; legacy spelling fields remain compatibility authority for existing paths.
- `publish_contract_plans()` now enriches regalloc output with placement identity before frame/call/storage plan publication. Manual tests that inspect placement after a direct `run_regalloc()` need to publish contract plans first.
- `PreparedRegisterSlotPool::ReservedScratch` is currently used for published call clobbers, derived from caller-saved spans, not as a full scratch allocator migration.
- Keep future migration slices semantic; do not replace string assertions with target-name-shaped special cases.

## Proof

Ran delegated proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_prepare_(liveness|frame_stack_call_contract)" --output-on-failure' > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful incremental build and both focused tests passing: `backend_prepare_liveness` and `backend_prepare_frame_stack_call_contract`.

Supervisor-side regression check on the matching focused before/after logs before the broader validation below replaced `test_after.log`:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed; before 2/2, after 2/2, no new failing tests. The default strict-increase guard mode reported only that the pass count did not increase, which is expected for this assertion-strengthening slice.

Supervisor-side broader prepared validation now preserved in `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^backend_prepare_" --output-on-failure' > test_after.log 2>&1`

Result: passed; 7/7 `backend_prepare_` tests passed.
