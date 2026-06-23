Status: Active
Source Idea Path: ideas/open/319_rv64_stack_homed_fused_compare_control_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Stack-Homed Fused Compare Evidence

# Current Packet

## Just Finished

Completed Step 1 evidence normalization for `src/00077.c` and `src/00143.c`.

Fresh probe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_319_step1/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_319_step1/summary.md`
- BIR dumps under `build/rv64_c_testsuite_probe_latest/triage_319_step1/bir/`
- prepared-BIR dumps under
  `build/rv64_c_testsuite_probe_latest/triage_319_step1/prepared/`
- emitted RV64 assembly under
  `build/rv64_c_testsuite_probe_latest/triage_319_step1/asm/`
- clang/qemu logs under
  `build/rv64_c_testsuite_probe_latest/triage_319_step1/logs/`

Fresh candidate status:

| case | BIR | prepared BIR | RV64 emit | clang link | qemu | classification |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `src/00077.c` | 0 | 0 | 0 | 0 | 0 | `supported-linked-pass` |
| `src/00143.c` | 0 | 0 | 0 | 1 | skipped | `link-failure` |

`src/00077.c` prepared fused-compare evidence:

- `branch_condition entry kind=fused_compare condition=%t5 compare=ne i32 %t4, 1000`
  consumes stack-homed `%t5` (`home %t5 ... kind=stack_slot`) in the entry
  conditional branch to `block_1` / `block_2`.
- `branch_condition block_2 kind=fused_compare condition=%t12 compare=ne i32 %t11, 1000`
  consumes `%t12` with stack-homed compare operand `%t11`.
- The emitted code links and qemu returns 0, so no current emitted-code failure
  remains for this candidate.

`src/00143.c` prepared fused-compare evidence:

- `branch_condition for.cond.1 kind=fused_compare condition=%t1 compare=slt i32 %t0, 39`
  consumes stack-homed `%t1` (`home %t1 ... kind=stack_slot`) in the loop
  condition branch to `block_1` / `block_2`.
- Later stack-homed fused compare consumers are also present:
  `for.cond.16` consumes `%t23`, `dowhile.cond.6` consumes `%t35`, and
  `block_16` consumes `%t83`.
- First emitted-code failure: the emitted first loop condition contains
  `blt t3, t4, .Lmain_block_1` followed by `j .Lmain_block_2`, but the emitted
  assembly defines `.Lmain_block_1` and never defines `.Lmain_block_2`. Clang
  fails with `Undefined temporary symbol .Lmain_block_2`.

## Suggested Next

Add focused expected-repair coverage for the `src/00143.c` pattern: a
stack-homed fused compare in a loop condition whose false successor must still
be emitted as a defined block label.

## Watchouts

- Do not special-case filenames, SSA names such as `%t5` or `%t1`, or observed
  stack offsets.
- `src/00077.c` now passes in fresh evidence; do not use it as a failing
  acceptance target unless it regresses.
- `src/00143.c` does not reach qemu because clang rejects the missing false
  successor label first.
- Do not fold nested select-chain/store-source, aggregate/byval,
  function-pointer, or external-call work into this route.

## Proof

Probe build ran:
`cmake --build --preset default -j`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. Fresh
proof log: `test_after.log`.
