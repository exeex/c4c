Status: Active
Source Idea Path: ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Destination-Home Shape Authority

# Current Packet

## Just Finished

Step 2, "Repair Destination-Home Shape Authority," completed as a narrow
implementation packet. Initial `git status --short` was clean. The prepared
value-home classifier now publishes an assigned stack slot as the canonical
home before computed `i32` immediate or pointer-base-plus-offset expression
homes. This gives move-bundle consumers explicit destination stack-slot id,
offset, size, and alignment when regalloc assigned stack storage.

Changed files:

- `src/backend/prealloc/regalloc/value_homes.cpp`
- `docs/rv64_gcc_torture_post_contract/prepared_authority_destination_home_step2.md`
- `todo.md`

Derived artifacts:

- `build/agent_state/552_step2_destination_home.allowlist`
- `build/agent_state/552_step2_destination_home/summary.tsv`
- `build/agent_state/552_step2_destination_home/failed.txt`
- `build/agent_state/552_step2_destination_home/row_status.tsv`

Focused Step 2 row counts:

| Classification | Rows |
| --- | ---: |
| Repaired and passing | 4 |
| Advanced to later RV64 global-data diagnostic | 4 |
| Advanced to later RV64 instruction-fragment diagnostic | 4 |
| Advanced to runtime mismatch | 3 |
| Advanced to other prepared authority gap | 1 |
| Still destination-home authority gap | 0 |

The one remaining prepared-authority row is `src/pr36339.c`: its destination
home is now `stack_slot`, but the remaining generic move-bundle diagnostic
lacks source type authority, so it belongs to the scalar type/size/alignment
family rather than destination-home publication.

## Suggested Next

Executor should run Step 3, "Repair Move Classification And Source-Home
Authority," starting with the 15 source-home or move-classification rows from
the Step 1 queue. Reuse the Step 2 row-status evidence so `src/pr36339.c`
remains reserved for the scalar type packet unless the supervisor chooses to
fold it into Step 4.

## Watchouts

- The Step 2 rule is intentionally canonical-home based. It does not create a
  second context-specific source/destination home for the same prepared value.
- `src/pr36339.c` no longer proves destination-home absence; it now points at
  missing source type authority.
- Four destination-home rows now route to RV64 global-data support, four to
  RV64 instruction-fragment support, and three to runtime mismatch. Those are
  not prepared destination-home blockers.

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step2_destination_home.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Build passed.
- Backend CTest passed `345/345`.
- Focused Step 2 proof passed `4/16`, with `0` rows still blocked by
  destination-home authority.

Proof output is preserved in `test_after.log`.
