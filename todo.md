Status: Active
Source Idea Path: ideas/open/411_rv64_object_route_rhs_bitfield_boolean_select_abort.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify The RHS Bitfield Boolean Abort

# Current Packet

## Just Finished

Lifecycle review closed 402 after the `20070212-2.c` frame-slot local-address
segfault family was repaired and the remaining `20000113-1.c` abort was
classified as a distinct runtime owner.

## Suggested Next

Execute Step 1: inspect the fresh `20000113-1.c` qemu/strace, prepared BIR,
and object disassembly evidence around `foobar`, `%t43/%t44/%t48`, and
`logic.end.40` to classify the first value scheduling/control-flow owner.

## Watchouts

- Do not reopen 402's `20070212-2.c` local-address repair without fresh
  regression evidence.
- Do not claim compile-only proof; this representative reaches qemu.
- Route bad prepared select/join facts to a producer owner instead of masking
  them in RV64 object emission.
- Do not use filename-specific handling, expectation rewrites, qemu weakening,
  unsupported downgrades, or allowlist filtering as progress.

## Proof

Lifecycle close/switch proof:

- Close gate: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.
