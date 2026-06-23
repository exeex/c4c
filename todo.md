Status: Active
Source Idea Path: ideas/open/311_rv64_ordinary_control_expression_completion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Complete Remaining Ordinary Tails

# Current Packet

## Just Finished

Step 5: Complete Remaining Ordinary Tails repaired the prepared RV64 loop
successor label/body publication boundary exposed by `src/00007.c` and
`src/00105.c`.

The semantic fix keeps RV64 emission lookups on prepared block-label IDs
instead of raw BIR IDs, so memory accesses and store-source publication records
for labels such as `for.latch.1` are found and latch/body blocks continue
emitting after a fused compare branch. The same packet also materializes
ordinary compare results when they are consumed as values, and adds prepared
integer `and`/`or`/`xor` emission so boolean tails such as `!(i != 0)` can feed
the next branch without stopping the successor body.

Focused coverage added
`backend_codegen_route_riscv64_prepared_loop_successor_body_boolean_tail`,
which checks a compact prepared `for` loop successor-body shape rather than a
c-testsuite filename contract.

Runtime probe scratch artifacts are under
`build/rv64_c_testsuite_probe_latest/triage_311_step5/`, including
`probe_results.tsv`, `summary.md`, and per-case emit/clang/QEMU artifacts.

| Case | Emit | Clang | QEMU | Classification |
| --- | ---: | ---: | ---: | --- |
| `src/00007.c` | `0` | `0` | `0` | `PASS` |
| `src/00105.c` | `0` | `0` | `0` | `PASS` |
| `src/00008.c` | `0` | `0` | `0` | `PASS` |
| `src/00030.c` | `0` | `0` | `0` | `PASS` |

## Suggested Next

Continue Step 5 with the remaining straight-line arithmetic/bitwise expression
result and return-tail gap shown by `src/00009.c` and `src/00027.c` through
`src/00029.c`.

## Watchouts

- Do not implement filename-shaped logic or c-testsuite-specific shortcuts.
- Do not downgrade supported-path expectations or mark cases unsupported.
- Keep stack/local address materialization and external-stub policy separate
  unless they directly block ordinary control/expression completion proof.
- The prepared fused-compare emitter currently relies on the existing RV64
  prepared value-home movement support; unsupported non-i32 or unsupported
  stack/address homes should remain separate blockers rather than being hidden.
- `src/00009.c`, `src/00027.c`, `src/00028.c`, and `src/00029.c` still
  assemble and then die with QEMU status `132` because the assembly ends after
  partial expression lowering with no result/return tail. These are ordinary
  expression completion gaps, not fused-compare branch emission failures.
- The old out-of-scope `backend_riscv_prepared_edge_publication` test failure
  still exists in the backend subset and is unchanged by this packet.
- `src/00143.c` should remain a later stress probe: it shares the repaired
  successor-label shape but also mixes large local arrays, pointer/address
  movement, and switch flow.

## Proof

Build and focused backend proof:

`cmake --build --preset default -j`

`ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_riscv64_prepared_(fused_compare|loop_successor_body_boolean_tail)'`

Focused result: `3/3` passed.

Backend regression guard shape:

`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

The backend subset still exits nonzero because of the pre-existing
`backend_riscv_prepared_edge_publication` failure. Regression comparison
against existing `test_before.log` passed:

`before: passed=217 failed=1 total=218`

`after: passed=218 failed=1 total=219`

`new failing tests: 0`

`result: PASS`

Targeted runtime probe command shape:

`./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/<case> -o <scratch>.s`

`clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler <scratch>.s -o <scratch>.bin -lm`

`timeout 5s qemu-riscv64 -L /usr/riscv64-linux-gnu <scratch>.bin`

Runtime result: `src/00007.c`, `src/00105.c`, `src/00008.c`, and
`src/00030.c` all passed emit, clang, and QEMU.
