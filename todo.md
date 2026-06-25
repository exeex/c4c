Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-audit Representative RV64 Object Route Rejections

# Current Packet

## Just Finished

Step 1 - Re-audit Representative RV64 Object Route Rejections: reran the
representative RV64 object-route allowlist and recorded the current first
rejecting layer for each case.

| Case | Current allowlist status | First rejecting layer | Key evidence | Audit conclusion |
| --- | --- | --- | --- | --- |
| `src/20000113-1.c` | fail | RV64 object emission prepared-function fragment construction | `case.log` reports `[RV64_C4C_OBJ_COMPILE_FAIL]` with `RISC-V backend object route unsupported prepared module shape`; `--dump-prepared-bir` succeeds and shows `foobar` with `frame_size=220`, many `frame_slot` value homes, `consumer_stack_to_stack` move bundles, and prepared CFG blocks `entry`, `logic.rhs.37`, `logic.rhs.end.39`, `logic.skip.38`, `logic.end.40`, `block_1`, `block_2`. The first unsupported body shape is a prepared `load_local` result whose home is a stack slot, while the current object fragment path requires a GPR result for `fragment_for_prepared_load_local`. | Prepared BIR/CFG reaches the handoff; the first blocker is target-owned RV64 object emission support for prepared stack-slot/local memory fragments, not a BIR shape rejection. |
| `src/20000205-1.c` | pass | none under this allowlist | `case.log` reports `[PASS][rv64-gcc-torture-backend-obj]`; `test_after.log` records `pass src/20000205-1.c`. | Currently passing under the representative allowlist only; do not overclaim broader recursive/multiblock coverage. |
| `src/20010119-1.c` | pass | none under this allowlist | `case.log` reports `[PASS][rv64-gcc-torture-backend-obj]`; `test_after.log` records `pass src/20010119-1.c`. | Currently passing under the representative allowlist only; do not overclaim broader call or extern coverage. |
| `src/20030216-1.c` | fail | RV64 object emission module-level global/storage gate | `case.log` reports `[RV64_C4C_OBJ_COMPILE_FAIL]` with `RISC-V backend object route unsupported prepared module shape`; `--dump-prepared-bir` succeeds and the source contains `const double one=1.0`; the current prepared text object module builder returns empty when `prepared.module.globals` or `prepared.module.string_constants` is non-empty. | Prepared BIR reaches the handoff; the first blocker is unsupported prepared global storage for non-i32 floating constant data. |

## Suggested Next

Execute a narrow target-owned RV64 object-emission packet for `src/20000113-1.c`:
teach the prepared object fragment path to lower the first unsupported
prepared stack-slot/local memory shape without reconstructing CFG semantics or
special-casing the testcase filename.

Narrow proof command:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true
```

## Watchouts

- `20000113-1.c` is the next best blocker because it is target-owned and
  exercises prepared stack/local memory plus existing block labels; keep the
  repair semantic-boundary preserving.
- `20030216-1.c` is a separate prepared global-storage/data blocker; do not
  mix float/global data support into the stack-slot object-fragment packet.
- The two passing cases are only current allowlist passes. Do not use them as
  proof of broad RV64 object-route completeness.
- Reject testcase-name shortcuts, expectation weakening, target-local CFG
  reconstruction, and silent block/edge dropping.

## Proof

Ran the supervisor-selected proof:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true
```

Result: completed with expected nonzero scan result captured by `|| true`;
`test_after.log` records 4 total, 2 passed, 2 failed. Log path:
`test_after.log`.
