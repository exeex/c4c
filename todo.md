Status: Active
Source Idea Path: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Pointer-Derived Load Address Rule

# Current Packet

## Just Finished

Plan Step 2 repaired the pointer-derived byte-offset multiply publication in
AArch64 dispatch so an immediate scale is materialized into a distinct scratch
instead of overwriting the live index/result carrier. The route handles both
register-homed and stack-homed multiply results, publishing stack-homed results
back to their prepared slot. A focused dispatch contract now rejects the old
`mov x9, #4; mul x9, x9, x9` shape and requires `ldr x9, [sp, #40]; mov x10,
#4; mul x9, x9, x10`.

`00181` advanced beyond the Step 1 timeout. In generated `Move`, the first
source and destination scan offsets now preserve the index carrier:
`ldr w9, [sp]; sxtw x9, w9; mov x10, #4; mul x9, x9, x10; str x9, [sp, #48];
add x9, x0, x9`, and the `dest[j]` scan has the same repaired shape at
`[sp, #96]`. The delegated proof now reaches a fast `[RUNTIME_NONZERO]
exit=Segmentation fault` instead of the prior 5 second timeout.

New localized residual for `00181`: after the bounded source scan false edge,
`Move` emits `mov x13, #0` in `.LBB193_16`, but the join `.LBB193_18` reloads
`ldr x13, [sp, #64]`, overwriting the false condition with the stale load slot.
The same select/join shape appears in the destination scan. This is separate
from pointer-add scaling and is the next first bad fact after the offset carrier
repair.

## Suggested Next

Localize and repair the prepared select/condition publication join in `Move`
so false scan exits keep the explicit zero/false value instead of reloading a
stale stack slot at the join.

## Watchouts

- Preserve idea 361's materialized pointer store writeback evidence: generated
  stores through `%t47` and `%t50` still exist; the pointer-offset fix was in
  the general immediate-scale multiply path, not a store-only special case.
- Preserve idea 360's correct starting state evidence; this packet did not
  touch starting-state expectations or `tests/c/external/c-testsuite/src/00181.c`.
- Do not special-case `00181`, `Move`, Hanoi tower globals, stack offsets, or
  select block labels when addressing the residual. The residual owner appears
  to be prepared select/condition value publication across the scan join.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result recorded in `test_after.log`: 6/7 tests passed. Passing:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c`. Residual:
`c_testsuite_aarch64_backend_src_00181_c` now fails in 1.66 seconds with
`[RUNTIME_NONZERO] exit=Segmentation fault`, advanced from the previous 5.01
second timeout. Also ran `git diff --check`, which passed.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
