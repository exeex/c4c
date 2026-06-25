Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Milestone Validation and Closure Decision

# Current Packet

## Just Finished

Completed Plan Step 4 representative validation for the simple-call and FPR ABI
edge packet.

The Step 4 representatives now pass without weakening runtime checks:
`src/20030125-1.c` passed in the earlier supervisor-run representative proof,
with the case artifact still recording
`[PASS][rv64-gcc-torture-backend-obj]`, and `src/20010119-1.c` passed in the
latest supervisor-run representative proof recorded in `test_after.log`.

## Suggested Next

Run Plan Step 5 milestone validation: the five-case representative allowlist
from `plan.md`, followed by the focused backend proof used by the prior steps.
Use the results to decide whether idea 358 can close or needs separate
follow-up ideas for any remaining non-overlapping boundaries.

## Watchouts

- Step 5 should prove all five current targets together:
  `src/20010119-1.c`, `src/20001203-1.c`, `src/20030216-1.c`,
  `src/20030125-1.c`, and `src/920410-1.c`.
- Keep representative validation semantic. Do not convert any remaining
  boundary into expectation downgrades, skipped runtime checks, or
  testcase-name matching.
- If milestone validation exposes a remaining boundary outside idea 358's mixed
  object-route ABI/width scope, record the split candidate instead of expanding
  this plan silently.

## Proof

No build was required for this todo-only update.

Referenced supervisor-run proof artifacts:

- `test_after.log`: `src/20010119-1.c` passed, summary `total=1 passed=1
  failed=0`.
- `build/rv64_gcc_c_torture_backend/src_20010119-1.c/case.log`: records
  `[PASS][rv64-gcc-torture-backend-obj]`.
- `build/rv64_gcc_c_torture_backend/src_20030125-1.c/case.log`: records
  `[PASS][rv64-gcc-torture-backend-obj]` from the earlier representative
  proof.

Suggested Step 5 proof:

```sh
tmp=$(mktemp); printf '%s\n' \
  src/20010119-1.c \
  src/20001203-1.c \
  src/20030216-1.c \
  src/20030125-1.c \
  src/920410-1.c > "$tmp"; \
CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh; \
rc=$?; rm -f "$tmp"; exit $rc
```

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```
