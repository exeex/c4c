Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify `00200` Timeout Mechanism

# Current Packet

## Just Finished

Completed `plan.md` Step 2 classification for
`c_testsuite_aarch64_backend_src_00200_c` without implementation edits.

First bad fact: the timeout is in backend-native AArch64 asm generation, not
in the generated program's runtime loop behavior. The source is the
`lshift-type.c` promotion/left-shift macro stress case; semantic and prepared
BIR for `main` contain a large straight-line expansion of nested
`do { ... } while (0)` checks whose conditions are constant false, and a
constant-false CFG walk reaches the final `printf`/`ret`. The existing
generated AArch64 binary also exits quickly when run directly, printing
`0 test(s) failed`.

Suspected owner: backend-native AArch64 codegen scalability for the expanded
shift/type-promotion CFG, likely in prepared lowering/regalloc/code emission
rather than runtime-output behavior, timeout policy, or runner behavior. A
secondary return-value publication mismatch exists in the stale generated
binary (`main` prints the expected output but returns a nonzero status), but
that is not the observed timeout mechanism.

Evidence path: `tests/c/external/c-testsuite/src/00200.c`;
`build/c_testsuite_aarch64_backend/src/00200.c.s`; focused
`--dump-bir`/`--dump-prepared-bir --mir-focus-function main`; bounded
`ctest` proof in `test_after.log`; direct diagnostic rerun of
`c4cll --codegen asm --target aarch64-unknown-linux-gnu` on `00200.c`, which
exceeded the CTest timeout window and was cleaned up.

## Suggested Next

Classify `00207` under `plan.md` Step 3 using the same bounded source,
generated-artifact, and stage-separation approach. Confirm whether its timeout
is compile/codegen time or generated-program runtime before assigning a
semantic owner.

## Watchouts

This is an umbrella classification plan. Do not make implementation, test,
expectation, runner, timeout-policy, CTest-registration, unsupported-list, or
proof-log-policy changes while this plan is active.

For `00200`, avoid classifying the residual as a runtime infinite loop unless
fresh evidence contradicts the stage split above. The existing generated binary
has a separate nonzero-return symptom, but the current CTest residual is
explained by backend asm generation exceeding the per-test timeout. Use
bounded timeout investigation and stale-process cleanup before trusting any
runtime logs.

## Proof

Ran the delegated bounded proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j1 --output-on-failure -R '^c_testsuite_aarch64_backend_src_00200_c$' --timeout 15 ; } > test_after.log 2>&1
```

Result: `test_after.log` records
`c_testsuite_aarch64_backend_src_00200_c` timing out after the configured
per-test timeout. Stale-process checks before and after the proof found no
leftover `ctest`, `cmake`, `c4cll`, `qemu`, or `00200.c.bin` process. One
manual diagnostic `c4cll --codegen asm` reproduction was stopped and cleaned up
after it exceeded the timeout window.
