Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the AArch64 prepared call argument ABI register numbering for
register-passed call arguments by deriving the physical ABI register slot from
the argument's register bank instead of the overall argument index. The shared
helper lives in `src/backend/prealloc/regalloc/call_return_abi.cpp`, and the
move/binding/plan surfaces now pass that per-bank slot into the existing target
register-name and placement helpers.

The `.str44` evidence moved from the old `x0`, `q1`, `q2` shape to the AAPCS64
shape for the representative variadic calls: generated `00204.c.s` now shows
`adrp/add x0, .str44`, then `mov v0.16b, ...` and `mov v1.16b, ...` before
`bl printf`. This preserves the repaired HFA lane-home and F128 carrier-view
transport while fixing the call-boundary placement.

One direct related source outside the original owned list was required:
`src/backend/prealloc/regalloc.cpp` publishes the prepared call ABI bindings
that `call_plans.cpp` consumes. Leaving that source on the old overall-index
rule kept stale q-register facts in prepared call plans.

## Suggested Next

Continue Step 2 with the separate target long-double literal/data
representation owner. The remaining first bad fact is still that selected
long-double bytes are x87-style 80-bit/padded payloads rather than AArch64
binary128 payloads before the now-correct q0/q1 `.str44` call consumes them.

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
or one stack offset. The ABI register numbering rule is now general per AArch64
register bank for prepared call-argument naming and placement; do not rewrite
expectations or paper over the separate long-double payload encoding mismatch.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`
passed. `c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_NONZERO` / segmentation fault after printing long-double mismatch
output. The `.str44` placement is repaired to q0/q1, so the remaining recorded
first bad fact is the separate long-double payload encoding mismatch. Also ran
`git diff --check`, which passed. `test_after.log` is the fresh proof log for
this dirty state.
