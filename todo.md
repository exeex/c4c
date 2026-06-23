Status: Active
Source Idea Path: ideas/open/311_rv64_ordinary_control_expression_completion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Complete Remaining Ordinary Tails

# Current Packet

## Just Finished

Step 4: Probe Nearby Runtime Candidates is complete. The targeted RV64
c-testsuite probe ran the delegated emit -> clang -> QEMU shape for
`src/00008.c`, `src/00030.c`, `src/00105.c`, and nearby same-bucket cases
`src/00006.c`, `src/00007.c`, `src/00009.c`, `src/00027.c`, `src/00028.c`,
`src/00029.c`, `src/00031.c`, and `src/00143.c`.

Scratch artifacts are under
`build/rv64_c_testsuite_probe_latest/triage_311_step4/`, including
`probe_results.tsv`, per-case assembly/binary/status/stdout/stderr artifacts,
and `summary.md`.

| Case | Previous | New | Classification |
| --- | --- | --- | --- |
| `src/00008.c` | `QEMU_NONZERO 139` | `PASS` | do/while fused-compare branch, block labels, and return tail now emit |
| `src/00030.c` | `QEMU_NONZERO 132` | `PASS` | call-result fused-compare branch chain now emits |
| `src/00105.c` | `QEMU_NONZERO 139` | `CLANG_FAIL` | fused compare now emits `blt`, but `.Lmain_block_1/.Lmain_block_2` are undefined |
| `src/00006.c` | `QEMU_NONZERO 132` | `PASS` | while fused-compare branch and return tail now emit |
| `src/00007.c` | `QEMU_NONZERO 132` | `CLANG_FAIL` | fused compare now emits `bne`, but loop successor labels are undefined |
| `src/00009.c` | `QEMU_NONZERO 132` | `QEMU_NONZERO 132` | straight-line expression chain stops before division/modulo/return tail |
| `src/00027.c` | `QEMU_NONZERO 132` | `QEMU_NONZERO 132` | bitwise OR/result/return tail still missing |
| `src/00028.c` | `QEMU_NONZERO 132` | `QEMU_NONZERO 132` | bitwise AND/result/return tail still missing |
| `src/00029.c` | `QEMU_NONZERO 132` | `QEMU_NONZERO 132` | bitwise XOR/result/return tail still missing |
| `src/00031.c` | `QEMU_NONZERO 132` | `PASS` | increment/decrement call-result predicate chain now emits |
| `src/00143.c` | `QEMU_NONZERO 132` | `CLANG_FAIL` | first loop compare now emits `blt`, but successor labels are undefined |

Outcome counts: `PASS=4`, `CLANG_FAIL=3`, `QEMU_NONZERO=4`,
`EMIT_FAIL=0`, `QEMU_TIMEOUT=0`. The primary representatives moved because
fused-compare branch emission now exists: `src/00008.c` and `src/00030.c`
pass, while `src/00105.c` moved from runtime failure to an assembler-visible
missing successor-label/body publication failure. Nearby movers include
`src/00006.c`, `src/00007.c`, `src/00031.c`, and `src/00143.c`.

## Suggested Next

Execute Step 5: complete the remaining ordinary tails exposed by this probe.
The tight next repair boundary is successor label/body publication for prepared
loop branches, using `src/00105.c` and `src/00007.c` as representatives. After
that, handle the separate straight-line arithmetic/bitwise expression result
and return-tail gap shown by `src/00009.c` and `src/00027.c` through
`src/00029.c`.

## Watchouts

- Do not implement filename-shaped logic or c-testsuite-specific shortcuts.
- Do not downgrade supported-path expectations or mark cases unsupported.
- Keep stack/local address materialization and external-stub policy separate
  unless they directly block ordinary control/expression completion proof.
- The prepared fused-compare emitter currently relies on the existing RV64
  prepared value-home movement support; unsupported non-i32 or unsupported
  stack/address homes should remain separate blockers rather than being hidden.
- `src/00143.c` also exposes missing successor labels after branch emission,
  but it mixes large local arrays, pointer/address movement, and switch flow;
  keep it as a later stress case rather than the first Step 5 repair target.
- `src/00009.c`, `src/00027.c`, `src/00028.c`, and `src/00029.c` still
  assemble and then die with QEMU status `132` because the assembly ends after
  partial expression lowering with no result/return tail. These are ordinary
  expression completion gaps, not fused-compare branch emission failures.
- The old out-of-scope `backend_riscv_prepared_edge_publication` test failure
  was not exercised by this no-CTest runtime probe.

## Proof

Delegated proof was a targeted runtime probe only; no broad CTest proof was
requested for this packet, and no `test_after.log` was produced or modified.

Per probed case, the command shape was:

`./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/<case> -o <scratch>.s`

`/usr/bin/clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler <scratch>.s -o <scratch>.bin -lm`

`timeout 5s /usr/bin/qemu-riscv64 -L /usr/riscv64-linux-gnu <scratch>.bin`

All 11 delegated cases completed the emit step. Clang failed only for
`src/00007.c`, `src/00105.c`, and `src/00143.c`, all with undefined temporary
successor labels `.Lmain_block_1` and `.Lmain_block_2`. QEMU was run under a
5-second timeout for all successfully linked cases; there were no timeouts.
