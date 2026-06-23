Status: Active
Source Idea Path: ideas/open/311_rv64_ordinary_control_expression_completion.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Sweep And Closure Readiness

# Current Packet

## Just Finished

Step 5: Complete Remaining Ordinary Tails repaired the straight-line prepared
RV64 signed division/remainder ordinary-tail gap exposed by `src/00009.c`.

Initial reprobe under
`build/rv64_c_testsuite_probe_latest/triage_311_step5_tail/` showed
`src/00027.c`, `src/00028.c`, and `src/00029.c` already passed after the prior
prepared bitwise slice, while `src/00009.c` still emitted and linked but died
with QEMU status `132`.

The selected missing operation was semantic `i32` signed division/remainder
value materialization in prepared scalar binary emission. The fix adds prepared
RV64 `sdiv`/`srem` lowering through `divw`/`remw` for `i32` results, preserves
existing value-home movement into stores and returns, and leaves unsigned or
non-owned tail families out of this packet. Focused coverage added
`backend_codegen_route_riscv64_prepared_signed_div_rem_return_tail`, a compact
store/load/div/rem/return-tail case rather than a c-testsuite filename
contract.

Runtime probe scratch artifacts are under
`build/rv64_c_testsuite_probe_latest/triage_311_step5_tail/`, including
`probe_results.tsv`, `probe_results_after.tsv`, regenerated BIR/prepared-BIR
for `00009`, and per-case emit/clang/QEMU artifacts. A supervisor rerun after
the final i32 scope tightening is under
`build/rv64_c_testsuite_probe_latest/triage_311_step5_tail_final/`.

| Case | Emit | Clang | QEMU | Classification |
| --- | ---: | ---: | ---: | --- |
| `src/00009.c` | `0` | `0` | `0` | `PASS` |
| `src/00027.c` | `0` | `0` | `0` | `PASS` |
| `src/00028.c` | `0` | `0` | `0` | `PASS` |
| `src/00029.c` | `0` | `0` | `0` | `PASS` |

## Suggested Next

Proceed to Step 6: Acceptance Sweep And Closure Readiness. Re-run the
supervisor-selected broader checkpoint and targeted runtime subset for idea 311,
then decide whether remaining failures are outside ordinary control/expression
completion or need a focused follow-up idea.

## Watchouts

- Do not implement filename-shaped logic or c-testsuite-specific shortcuts.
- Do not downgrade supported-path expectations or mark cases unsupported.
- Keep stack/local address materialization and external-stub policy separate
  unless they directly block ordinary control/expression completion proof.
- The prepared fused-compare and signed div/rem emitters currently rely on the
  existing RV64 prepared value-home movement support; unsupported non-i32,
  unsupported stack/address homes, and unsigned div/rem tails should remain
  separate blockers unless the Step 6 sweep proves they are required for this
  source idea.
- The old out-of-scope `backend_riscv_prepared_edge_publication` test failure
  still exists in the backend subset and is unchanged by this packet.
- `src/00143.c` should remain a later stress probe: it shares the repaired
  successor-label shape but also mixes large local arrays, pointer/address
  movement, and switch flow.

## Proof

Build and focused backend proof:

`cmake --build --preset default -j`

`ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_riscv64_prepared_signed_div_rem_return_tail'`

Focused result: `1/1` passed.

Backend regression guard shape:

`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

The backend subset still exits nonzero because of the pre-existing
`backend_riscv_prepared_edge_publication` failure. Regression comparison
against existing `test_before.log` passed:

`before: passed=218 failed=1 total=219`

`after: passed=219 failed=1 total=220`

`new failing tests: 0`

`result: PASS`

Targeted runtime probe command shape:

`./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/<case> -o <scratch>.s`

`clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler <scratch>.s -o <scratch>.bin -lm`

`timeout 5s qemu-riscv64 -L /usr/riscv64-linux-gnu <scratch>.bin`

Runtime result: `src/00009.c`, `src/00027.c`, `src/00028.c`, and
`src/00029.c` all passed emit, clang, and QEMU in the executor probe and the
supervisor final rerun.
