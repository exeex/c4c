Status: Active
Source Idea Path: ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Build Semantic Root-Cause Buckets

# Current Packet

## Just Finished

Step 3: Capture Representative Backend Evidence inspected the existing
`build/rv64_c_testsuite_probe_latest/asm` outputs and generated targeted
read-only debug artifacts under
`build/rv64_c_testsuite_probe_latest/triage_step3/` for the four representative
runtime classes. All selected `*.qemu.out` files remain zero-byte, so the QEMU
outcome is the TSV-recorded class/status.

| Case | QEMU outcome | Source shape | Backend evidence |
| --- | --- | --- | --- |
| `src/00005.c` | `QEMU_NONZERO`, status `132` | local `int`, `int *`, `int **`, pointer-to-pointer store | BIR/prepared BIR contain the expected conditional returns and stack slots, but both saved and regenerated assembly stop after `ld s2, 8(sp)` / `lw s2, 0(sp)` with no branch, epilogue, or `ret`. |
| `src/00008.c` | `QEMU_NONZERO`, status `139` | decrementing `do while (x)` loop | BIR/prepared BIR contain the loop backedge and final return, but assembly stops in `.Lmain_dowhile_cond_1` after `lw t0, 0(sp)` with no condition branch or return. |
| `src/00112.c` | `QEMU_NONZERO`, status `112` | string literal pointer compared with null | BIR/prepared BIR lower `%t2 = bir.eq ptr @.str0, 0` and `ret %t2`; assembly emits `.str0` but returns `t0` directly via `mv a0, t0; ret`, leaving the comparison result unmaterialized. |
| `src/00025.c` | `QEMU_TIMEOUT`, status `124` | call to external `strlen("hello")`, return minus 5 | Prepared BIR records a direct external fixed-arity call plan for `strlen`; assembly calls `strlen` and then defines `.globl strlen` / `strlen:` with no body after `main`, so the external-call path has no callable implementation/stub in the emitted text. |

Scratch evidence paths include `summary.md` plus per-case `.bir.txt`,
`.prepared-bir.txt`, `.regen.s`, `.status`, and empty `.err` files in
`build/rv64_c_testsuite_probe_latest/triage_step3/`. Each targeted
`--dump-bir`, `--dump-prepared-bir --mir-focus-function main`, and
`--codegen asm` command exited `0` under a 30s timeout guard.

## Suggested Next

Start Step 4 from `plan.md`: build semantic root-cause buckets for the 123
runtime-failure rows by comparing nearby cases against these evidence shapes:
truncated conditional/loop assembly, unmaterialized pointer comparison results,
and unresolved external-call emission.

## Watchouts

This active idea is triage-only. Do not implement runtime fixes, weaken test
contracts, or turn the full 220-case sweep into mandatory CTest coverage. The
Step 3 representative evidence suggests the runtime statuses are symptoms of
emitted assembly completeness/materialization/call-stub gaps rather than QEMU
diagnostic-bearing failures. The Step 4 buckets should stay semantic and avoid
case-name matching.

## Proof

No build or CTest proof required for this triage-only evidence update. Targeted
read-only commands were run for `tests/c/external/c-testsuite/src/00005.c`,
`00008.c`, `00112.c`, and `00025.c`:
`./build/c4cll --dump-bir --target riscv64-linux-gnu <case>`,
`./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function main <case>`,
and `./build/c4cll --codegen asm --target riscv64-linux-gnu <case> -o build/rv64_c_testsuite_probe_latest/triage_step3/<case>.regen.s`.
No `test_after.log` was produced because the delegated proof explicitly did not
require build/ctest proof.
