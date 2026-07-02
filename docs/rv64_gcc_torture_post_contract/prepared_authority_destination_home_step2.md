# Prepared Authority Step 2 Destination-Home Repair

Status: Step 2 implementation packet for
`ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`.

## Rule

Prepared value-home publication now treats an assigned stack slot as the
canonical home before publishing computed `i32` immediates or pointer
base-plus-offset expression homes. This keeps destination storage/home
agreement explicit: when regalloc has assigned a value to a stack slot, the
prepared layer publishes the stack slot id, offset, size, and alignment as the
auditable destination home.

The rule is storage-authority based. It does not inspect testcase names,
expected assembly, RV64 register spelling, or RV64 lowering diagnostics.

## Focused Rows

The Step 2 allowlist was derived from the Step 1 queue where
`earliest_missing_fact_group=destination_home`:

- `build/agent_state/552_step2_destination_home.allowlist`
- `build/agent_state/552_step2_destination_home/summary.tsv`
- `build/agent_state/552_step2_destination_home/failed.txt`
- `build/agent_state/552_step2_destination_home/row_status.tsv`

## Results

Focused proof result:

```text
[rv64-gcc-torture] total=16 passed=4 failed=12
```

Row classifications after the repair:

| Classification | Rows |
| --- | ---: |
| Repaired and passing | 4 |
| Advanced to later RV64 global-data diagnostic | 4 |
| Advanced to later RV64 instruction-fragment diagnostic | 4 |
| Advanced to runtime mismatch | 3 |
| Advanced to other prepared authority gap | 1 |
| Still destination-home authority gap | 0 |

The remaining prepared-authority row is `src/pr36339.c`: its destination home
is now `stack_slot`, but the current generic move-bundle diagnostic still lacks
source type authority, so it belongs with the scalar type/size/alignment family
rather than destination-home publication.

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step2_destination_home.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Build passed.
- Backend CTest passed `345/345`.
- Focused allowlist passed `4/16`; all 12 failures moved beyond the original
  destination-home authority gap.

Proof output is preserved in `test_after.log`.
