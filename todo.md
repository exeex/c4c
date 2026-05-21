Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Classify `00207` Timeout Mechanism

# Current Packet

## Just Finished

Completed `plan.md` Step 3 classification for
`c_testsuite_aarch64_backend_src_00207_c` without implementation edits.

First bad fact: the timeout is generated-program runtime, not compile/codegen
time. The delegated CTest proof refreshed both
`build/c_testsuite_aarch64_backend/src/00207.c.s` and
`build/c_testsuite_aarch64_backend/src/00207.c.bin` before timing out, and a
direct bounded run of the generated binary repeatedly printed `boom!` until
`timeout` killed it with rc 124. The source expects exactly two `boom!` lines
before `f3` prints `11`, `12`, `0`, and `1`.

Suspected owner: AArch64 backend dynamic stack/VLA frame lowering, specifically
stable addressing for fixed stack homes across a moving `sp`. In `f1`, the
argument `argc` is initially spilled at `[sp, #8]`; after the VLA allocation
moves `sp`, the loop reload/update for `argc-- == 0` still uses `[sp, #8]`
and `[sp, #24]` relative to the adjusted stack pointer, so the loop control
does not reliably reach zero and the label path keeps printing `boom!`.

Evidence path: `tests/c/external/c-testsuite/src/00207.c`;
`tests/c/external/c-testsuite/src/00207.c.expected`;
`build/c_testsuite_aarch64_backend/src/00207.c.s` lines 5-13 and 24-46;
bounded `ctest` proof in `test_after.log`; direct diagnostic run
`timeout 3s build/c_testsuite_aarch64_backend/src/00207.c.bin`, which emitted
repeated `boom!` lines and exited 124.

## Suggested Next

Pick the next residual timeout bucket from the active plan and classify it with
the same source/artifact/stage-separation approach before assigning a semantic
owner.

## Watchouts

This is an umbrella classification plan. Do not make implementation, test,
expectation, runner, timeout-policy, CTest-registration, unsupported-list, or
proof-log-policy changes while this plan is active.

For `00207`, do not treat the residual as backend compile/codegen slowness:
fresh proof shows asm/bin emission completed before the CTest timeout. The
runtime loop is tied to VLA/dynamic-stack lowering in `f1`; avoid solving this
through timeout policy, expectation changes, CTest registration changes, or
filename-specific handling.

## Proof

Ran the delegated bounded proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j1 --output-on-failure -R '^c_testsuite_aarch64_backend_src_00207_c$' --timeout 15 ; } > test_after.log 2>&1
```

Result: `test_after.log` records
`c_testsuite_aarch64_backend_src_00207_c` timing out after the configured
per-test timeout. The proof was sufficient for stage classification because it
refreshed the generated asm and binary before the timeout; direct bounded
runtime confirmation showed the generated binary looping on `boom!`. Stale
process checks before and after the proof found no leftover relevant `ctest`,
`cmake`, `c4cll`, `qemu`, or `00207.c.bin` process.
