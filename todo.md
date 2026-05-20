Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Byval Aggregate Slot Accounting Residual

# Current Packet

## Just Finished

Lifecycle routed the fixed non-HFA byval aggregate residual out of idea 326
and reactivated idea 328. The prior packet evidence is preserved in
`ideas/open/326_aarch64_variadic_hfa_floating_residual.md`,
`ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`,
and `plan.md`.

## Suggested Next

Execute Step 1: localize AAPCS64 fixed aggregate arguments across the caller
and callee boundary for the `s8, s9, s10, s11, s12, s13` shape, including the
register-to-stack transition when too few GPR argument registers remain for a
whole aggregate.

Smallest next proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|c_testsuite_aarch64_backend_src_00204_c)$'`.

Focused generated-state probes to reuse:
`./build/c4cll --target aarch64-linux-gnu --dump-bir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c | sed -n '560,630p'`,
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function arg --mir-focus-value t103 tests/c/external/c-testsuite/src/00204.c | sed -n '1,220p'`,
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function fa1 tests/c/external/c-testsuite/src/00204.c | sed -n '1,220p'`, and
`./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00204.c | sed -n '/^arg:/,/^fr_s1:/p' | rg -n -C 35 'bl fa1|s8|s9|s10|s11|s12|s13|ldr x[0-7]|str x[0-7]|mov x[0-7]'`.

## Watchouts

Do not continue this residual under the HFA/floating idea. The new bad line
involves non-HFA `char[]` structs and matches a fixed aggregate byval
call-boundary placement problem.

Do not repair this with one function, one string literal, one register, one
stack offset, or one emitted instruction sequence. The real contract is the
general AAPCS64 rule for small non-HFA aggregates: each aggregate consumes its
own rounded register/stack slots, and an aggregate that cannot fit in the
remaining GPR argument registers must transition coherently to stack passing
for both caller publication and callee entry materialization.

The previous singleton HFA owner remains fixed; do not reopen HFA/floating
return publication, stdarg cursor/format, `va_start` overflow cursor
initialization, non-HFA aggregate `va_arg` materialization, fixed-formal entry
publication, local/value-home publication, or frame/formal publication unless
fresh generated evidence moves the first bad fact back to one of those owners.

## Proof

Lifecycle-only switch. No implementation validation was run.

Close gate for idea 326 was checked with:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`.

Result: close rejected for the available log basis. Both logs report
`passed=3 failed=1 total=4`, so the guard failed with non-increasing pass
count. Idea 326 remains open but inactive with a handoff note.
