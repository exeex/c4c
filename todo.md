Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Prepared Allocation Records Away From Authoritative Strings

# Current Packet

## Just Finished

Step 3 continued the prepared record migration by publishing structured `PreparedRegisterPlacement` on move-resolution, ABI-binding, and allocation-constraint records at creation time. `regalloc` now carries placement identity through value moves, immediate materialization moves, call argument/result moves, function return moves, call-site ABI bindings, and fixed/preferred/forbidden allocation constraint pools. Duplicate/no-op checks prefer placement equality when both sides have structured placement.

`prepared_printer` now prints those move/binding placement fields before legacy `reg=` text, keeping the spelling as display/compatibility output.

Focused liveness, frame/call, and printer tests now fail if call-site ABI bindings, call-result moves, or call-crossing allocation constraints lose the structured placement fields.

## Suggested Next

Next coherent packet: either finish Step 3 with a targeted audit of remaining prealloc string-authoritative comparisons, or move to Step 4 by converting a narrow AArch64 prepared-to-MIR path to consume `PreparedRegisterPlacement` before legacy register spelling.

## Watchouts

- This packet intentionally did not rewrite target profiles or downstream target consumers; legacy spelling fields remain compatibility display for existing paths.
- Prepared-printer summary/detail text now includes extra `placement=` or `spill_slot=` tokens; future string checks should assert those structured tokens for migrated records instead of relying only on `reg=` or `slot_id=`.
- Move-resolution and allocation-constraint records still preserve legacy register-name vectors for compatibility, but call/return coalescing no longer relies on the string when both structured placements are available.
- `PreparedRegisterSlotPool::ReservedScratch` is currently used for published call clobbers, derived from caller-saved spans, not as a full scratch allocator migration.
- Keep future migration slices semantic; do not replace string assertions with target-name-shaped special cases.

## Proof

Ran delegated proof:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_(prepared_printer|prepare_frame_stack_call_contract)$" --output-on-failure' > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful incremental build and both focused tests passing: `backend_prepare_frame_stack_call_contract` and `backend_prepared_printer`.

Supervisor-side regression check on the matching focused before/after logs before the broader validation below replaced `test_after.log`:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed; before 2/2, after 2/2, no new failing tests.

Supervisor-side broader prepared validation now preserved in `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^backend_prepare_|^backend_prepared_printer$" --output-on-failure' > test_after.log 2>&1`

Result: passed; 8/8 prepared tests passed.

Post-commit baseline review found the full-suite candidate regressed two prepared BIR CLI dump snippet tests because the new structured `placement=` token now appears between `bank=gpr` and the legacy `reg=` text. The candidate baseline was rejected, and the CLI expectations were updated to assert the structured placement identity instead of weakening the checks.

Supervisor-side prepared BIR CLI dump validation now preserved in `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build --output-on-failure -R "^backend_cli_dump_prepared_bir_"' > test_after.log 2>&1`

Result: passed; 10/10 prepared BIR CLI dump tests passed.

Ran focused proof for move/binding placement migration:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_(prepared_printer|prepare_frame_stack_call_contract)$" --output-on-failure' > test_after.log 2>&1`

Result: passed; both focused tests passed after adding assertions for structured ABI-binding and call-result move placement.

Ran broader prepared plus prepared BIR CLI dump validation, now preserved in `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^backend_prepare_|^backend_prepared_printer$|^backend_cli_dump_prepared_bir_" --output-on-failure' > test_after.log 2>&1`

Result: passed; 18/18 tests passed.

Ran the same broader validation after adding structured allocation-constraint placement companions:

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^backend_prepare_|^backend_prepared_printer$|^backend_cli_dump_prepared_bir_" --output-on-failure' > test_after.log 2>&1`

Result: passed; 18/18 tests passed.

Formatting note: `clang-format` is not installed in this workspace, so no formatter pass was available.
