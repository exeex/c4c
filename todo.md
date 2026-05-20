Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 localization advanced the remaining `.str44` failure past HFA lane-home
publication, F128 carrier-view transport, and prepared frame-slot address
retargeting. The generated `.str44` paths now load selected long-double HFA
lanes from concrete prepared homes, for example `%hfa31` copies the selected
16-byte lane into `sp + 192` / `sp + 200`, loads `ldr q13, [sp, #192]`,
stores the local F128 object home at `sp + 32`, reloads from `sp + 32`, and
emits concrete q call-boundary moves. `%hfa32` similarly loads selected lanes
from `sp + 208` and `sp + 224` before the q moves.

Two precise bad facts remain and both sit outside this packet's currently
owned dirty files. First, the selected long-double bytes are not AArch64
binary128 bytes. Clang for `--target=aarch64-linux-gnu` emits `31.1L` as
binary128 xwords `0x999999999999999a` and `0x4003f19999999999`, i.e.
little-endian bytes `9a 99 99 99 99 99 99 99 99 99 99 99 99 f1 03 40`.
The generated `hfa31` object in `00204.c.s` is instead bytes
`99 99 99 99 99 f1 03 40 00 00 00 00 00 00 00 a0`, an x87-style
80-bit/padded payload. The `va_arg` selection copies those bytes through the
register-save-area or overflow path into the now-correct homes, so payload
contents are already wrong before the `.str44` call consumes them.

Second, the observed AAPCS64 call-boundary placement for
`printf("%.1Lf,%.1Lf", ...)` is off by one in the vector register bank.
A Clang AArch64 reference call with one GPR format argument and two
long-double varargs passes the long doubles in `q0` and `q1`. The prepared
call plan for `myprintf` block 43, instruction 5 records `arg0` as `x0` but
records `arg1` / `arg2` as `q1` / `q2`, and generated assembly follows that
with `mov v1.16b, ...` and `mov v2.16b, ...`. The exact owner surface for the
ABI register numbering is prepared call ABI planning/move generation, notably
`src/backend/prealloc/regalloc/call_return_abi.cpp`,
`src/backend/prealloc/regalloc/call_moves.cpp`, and
`src/backend/prealloc/call_plans.cpp`; those files were not delegated as owned
files for this packet. The long-double byte owner is the target long-double
constant/data representation path, likely around the LIR/constant-data
emission surfaces that choose `fp128` versus x87 payload bytes.

## Suggested Next

Continue Step 2 only after the supervisor chooses the next owner. The smallest
backend packet is to repair AArch64 prepared call argument ABI register
numbering so FPR/Vreg argument registers are counted per register bank rather
than by the overall argument index. A separate packet is needed for the target
long-double literal/data representation mismatch if this plan owns that
frontend/LIR surface.

## Watchouts

Do not reopen the closed non-HFA aggregate string materialization route unless
fresh generated-code evidence shows selected `%7s` / `%9s` bytes are again
missing before their observing `printf` calls. The Step 2 code changes are
intended to affect only HFA-classified aggregate `va_arg` plans with prepared
lane homes.

The calls-owned F128 call-boundary adjustments, F128 carrier-view transport,
and prepared frame-slot address retargeting now produce `.str44` argument
publication plus selected q-register transport from concrete stack homes. Do
not special-case `%hfa31`, `%hfa32`, `.str44`, one register, one frame slot id,
or one stack offset. The next backend repair should be a general AArch64
per-bank call ABI register index rule; it should not rewrite expectations or
paper over the separate long-double payload encoding mismatch.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`
passed. `c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_NONZERO` / segmentation fault. The failure is now localized to the
two concrete bad facts above: selected long-double bytes are x87-style
80-bit/padded rather than AArch64 binary128, and the prepared `.str44`
variadic `printf` ABI placement uses `q1` / `q2` where Clang uses `q0` /
`q1`. `test_after.log` is the fresh proof log for this dirty state.
