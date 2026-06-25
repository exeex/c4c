Status: Active
Source Idea Path: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Full Two-Pass Roundtrip Stability

# Current Packet

## Just Finished

Step 5 - Prove Full Two-Pass Roundtrip Stability: recorded supervisor full-suite
proof that the broad RV64I plus EV64 roundtrip contract now passes through
regular full CTest. The committed corpus reaches full `input.s -> pass1.o ->
pass1.s -> pass2.o -> pass2.s -> pass3.o` stability under the normal test
suite, and the regression guard comparison against `test_baseline.log` passed
with 3351/3351 tests before and after and no new tests over 30 seconds.

## Suggested Next

Proceed to Step 6 for closure review and the supervisor-owned broader
validation/acceptance decision.

## Watchouts

- External relocation/fixup semantics remain unsupported and are still
  non-goals for this runbook slice.
- Local same-file branch/`jal` roundtrip relies on truthful object labels;
  `c4c-objdump` still fails closed instead of fabricating labels for arbitrary
  PC-relative targets.
- Step 5 is proof recording only; lifecycle closure and final acceptance remain
  supervisor/plan-owner decisions.

## Proof

Passed by supervisor:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log
```

Regression guard against `test_baseline.log` passed with 3351/3351 tests before
and after, with no new tests over 30 seconds.

Proof log: `test_after.log`.
