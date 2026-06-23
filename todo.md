Status: Active
Source Idea Path: ideas/open/311_rv64_ordinary_control_expression_completion.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Sweep And Closure Readiness

# Current Packet

## Just Finished

Step 6: Acceptance Sweep And Closure Readiness ran the targeted RV64
c-testsuite acceptance sweep for the idea 311 ordinary control/expression
completion family.

Scratch artifacts are under
`build/rv64_c_testsuite_probe_latest/triage_311_step6_acceptance/`, including
`probe_results.tsv`, per-case emitted assembly, clang stdout/stderr, QEMU
stdout/stderr, and regenerated BIR/prepared-BIR evidence for the one residual
runtime failure.

| Case | Emit | Clang | QEMU | Classification |
| --- | ---: | ---: | ---: | --- |
| `src/00006.c` | `0` | `0` | `0` | `PASS` |
| `src/00007.c` | `0` | `0` | `0` | `PASS` |
| `src/00008.c` | `0` | `0` | `0` | `PASS` |
| `src/00009.c` | `0` | `0` | `0` | `PASS` |
| `src/00027.c` | `0` | `0` | `0` | `PASS` |
| `src/00028.c` | `0` | `0` | `0` | `PASS` |
| `src/00029.c` | `0` | `0` | `0` | `PASS` |
| `src/00030.c` | `0` | `0` | `0` | `PASS` |
| `src/00031.c` | `0` | `0` | `0` | `PASS` |
| `src/00105.c` | `0` | `0` | `0` | `PASS` |
| `src/00143.c` | `0` | `0` | `132` | `QEMU_NONZERO` |

The remaining `src/00143.c` failure is closure-ready as an out-of-scope stress
residual for idea 311. The emitted assembly now reaches the first loop
condition but stops after loading `n` and publishing the value to a stack home;
the prepared BIR then expands indexed local array writes and reads into large
`i16` element-select/update chains, pointer/address-offset local access forms,
and later switch-shaped flow. That is not the ordinary fused-compare,
successor-body, bitwise boolean-tail, or signed div/rem return-tail mechanism
owned by this runbook.

## Suggested Next

Proceed to lifecycle closure review for idea 311. If the supervisor wants to
keep mining `src/00143.c`, use a separate follow-up focused on local
array/address materialization and expanded select chains rather than extending
this ordinary control/expression completion runbook.

## Watchouts

- Do not implement filename-shaped logic or c-testsuite-specific shortcuts.
- Do not downgrade supported-path expectations or mark cases unsupported.
- Keep `src/00143.c` separate unless explicitly opening local array/address
  materialization work; it mixes expanded local arrays, pointer/address-offset
  access, `i16` select chains, and switch-shaped flow.
- The old out-of-scope `backend_riscv_prepared_edge_publication` test failure
  still exists in the backend subset and is unchanged by this packet.
- No fresh full backend guard was run in this validation-only packet; the
  supervisor-provided current guard baseline remains `test_before.log` after
  commit `54155bca`.

## Proof

Targeted runtime probe command shape:

`./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/<case> -o <scratch>.s`

`clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler <scratch>.s -o <scratch>.bin -lm`

`timeout 5s qemu-riscv64 -L /usr/riscv64-linux-gnu <scratch>.bin`

Runtime result: 10 of 11 delegated cases passed emit, clang, and QEMU. The
only residual is `src/00143.c`, classified as `QEMU_NONZERO` status `132` and
out of scope for idea 311.

Focused backend sanity:

`ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_prepared_(fused_compare|loop_successor|signed_div_rem)'`

Focused result: `6/6` passed.

This validation-only packet did not rewrite `test_after.log`; the canonical
backend guard state remains with the supervisor-provided current
`test_before.log` baseline.
