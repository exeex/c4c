Status: Active
Source Idea Path: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Pointer-Derived Load Scaling Boundary

# Current Packet

## Just Finished

Plan Step 1 localized the pointer-derived load/address scaling boundary for
`00181` without implementation edits. The delegated focused proof reproduced
the expected residual: the backend contracts plus `00170` and `00189` pass,
while `c_testsuite_aarch64_backend_src_00181_c` times out after 5 seconds.

First bad fact: in `Move`, the source operation `while (i<N && source[i]==0)`
lowers through semantic/prepared BIR as `%t8 -> %t9 = sext i -> %t10.byte_offset
= bir.mul i64 %t9, 4 -> %t10 = bir.add ptr %p.source,
%t10.byte_offset -> %t11 = bir.load_local ... addr %t10`. The expected address
is `%p.source + ((int64)i * 4)`. Generated AArch64 for the same load reloads
`i`, sign-extends it, then overwrites the index carrier with the scale constant:
`sxtw x9, w9; ... ldr x9, [sp, #40]; mov x9, #4; mul x9, x9, x9; add x9, x0,
x9; ldr w9, [x9]`. That emits `%p.source + 16` for every `i`, instead of
`%p.source + i * 4`.

Adjacent same-shape evidence: the `dest[j]` scan has the same bad offset
calculation for `%t30` (`mov x9, #4; mul x9, x9, x9; add x9, x1, x9`), and the
`dest[j-1]` store address `%t47` also computes a constant `16` byte offset
instead of `(j - 1) * 4`. The later body load/store/return shapes `%t42`,
`%t50`, and `%t54` preserve the index in a different register before multiplying
by `4`, so the first wrong boundary is specifically the AArch64 lowering of the
prepared pointer-add byte-offset multiply when the selected destination
overwrites the live index carrier.

Timeout-causing consumer path: after the first two printed moves, runtime hangs
inside the next `Move` scan. The bad `source[i]` load ignores the actual
`source[0..3]` elements, repeatedly reads `%p.source + 16`, and feeds the
`%t11 == 0` / `%t16` / `%t17` loop condition for `while (i<N &&
source[i]==0)`. When called on an empty/adjacent tower during recursion, this
bad load plus the prepared select/skip consumer path keeps the scan progressing
instead of cleanly reaching the bounded post-scan move path, producing the
observed 5 second timeout before the next `PrintAll`.

## Suggested Next

Execute plan Step 2: repair the general AArch64 pointer-add byte-offset
materialization so `base + index * scale` keeps the live index carrier distinct
from the scale immediate/register. Add focused backend coverage for this shape
before using `00181` as proof.

## Watchouts

- Preserve idea 361's materialized pointer store writeback evidence: generated
  stores through `%t47` and `%t50` still exist, but `%t47` shares the same
  constant-offset address bug and should be fixed through the general
  pointer-add rule, not as a store-only special case.
- Preserve idea 360's correct starting state; unbuffered runtime output still
  shows the initial `A: 1 2 3 4`, `B: 0 0 0 0`, `C: 0 0 0 0` state before the
  timeout.
- Do not special-case `00181`, `Move`, Hanoi tower globals, stack offsets, or
  the emitted `mov x9, #4; mul ...` neighborhood. The repair owner is the
  register/value selection for pointer-derived address byte-offset
  materialization.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result recorded in `test_after.log`: 6/7 tests passed;
`c_testsuite_aarch64_backend_src_00181_c` timed out after 5.01 seconds. This is
sufficient for the localization-only packet and intentionally leaves the
implementation unrepaired.
