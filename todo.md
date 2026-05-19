Status: Active
Source Idea Path: ideas/open/309_aarch64_indirect_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Callee and Argument Preservation

# Current Packet

## Just Finished

Plan Step 2 repaired the remaining AArch64 indirect-call preservation path for
`00189.c`. Dispatch now retargets ordinary symbol-load results back to the
explicit prepared register home, keeps call results available from the ABI
result register for the next call setup, orders before-call moves so live source
registers are consumed before destination clobbers, and rematerializes
frame-slot call arguments from same-block producers when the frame-slot home
would be stale. The focused assembly now loads `fprintfptr` into the same
register used by `blr`, moves the nested result out of `x0` before setting the
outer `x0`, and reloads `stdout` into `x0` before the final indirect call.

## Suggested Next

Supervisor can review and commit this Step 2 slice, then proceed to Step 3
focused runtime proof/close evaluation if no broader validation is requested.

## Watchouts

The repaired path intentionally avoids relying on the stale stack homes observed
for `stdout` and the nested call result. The stack/save-slot overlap still looks
like a lower-level prepared-frame boundary, but this packet did not broaden into
stack-layout ownership because the AArch64 call-boundary lowering can preserve
the live values from authoritative producer and ABI-register facts.

## Proof

Delegated proof ran from the repo root:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_machine_printer|c_testsuite_aarch64_backend_src_00189_c)$' > test_after.log 2>&1`

Result: `test_after.log` reports 5/5 passing, including
`c_testsuite_aarch64_backend_src_00189_c`. This improves over the captured
baseline where 4/5 passed and `00189.c` segfaulted.

Supervisor validation after review:

- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log` passed, resolving the focused `00189.c` failure.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  139/139.
