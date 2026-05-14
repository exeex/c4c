Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Prepared Allocation Records Away From Authoritative Strings

# Current Packet

## Just Finished

Step 3 started the prepared-printer consumer migration by rendering structured placement identity on representative prepared allocation records before the legacy register or slot spelling. `prepared_printer` now prints register placement (`bank:pool#slot/wN`) and spill-slot placement (`slot#N+stackOFFSET`) for saved registers, call argument/result records, clobbers, preserved values, storage plans, and spill/reload records while leaving existing `reg=`, `units=`, `slot_id=`, and stack-offset compatibility text in place.

Focused prepared-printer tests now fail if representative call argument/result, clobber, preserved-value, storage-plan, or spill/reload records lack the structured placement fields.

## Suggested Next

Next coherent packet: migrate another narrow prepared consumer family to inspect `PreparedRegisterPlacement`/`PreparedSpillSlotPlacement` first, leaving legacy spelling fields as compatibility display or fallback. Keep this limited to pre-target prepared surfaces unless the supervisor explicitly opens a target handoff packet.

## Watchouts

- This packet intentionally did not rewrite `regalloc.cpp`, target profiles, or downstream target consumers; legacy spelling fields remain compatibility display for existing paths.
- Prepared-printer summary/detail text now includes extra `placement=` or `spill_slot=` tokens; future string checks should assert those structured tokens for migrated records instead of relying only on `reg=` or `slot_id=`.
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
