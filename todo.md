Status: Active
Source Idea Path: ideas/open/649_pointer_global_local_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Run Broader Validation And Close Or Park

# Current Packet

## Just Finished

Recorded Step 5 broader RV64 validation for idea 649.

The earlier Step 5 regression-guard blocker in
`backend_cli_riscv64_local_string_constant_label_pointer_compound_arg` was
repaired before this broader result was accepted. That local string-label
compound case no longer appears as a new failure.

Focused and representative proof remain accepted:

- The focused pointer/global local-publication dump and object tests passed.
- The focused live-load expected-failure test passed, preserving fail-closed
  behavior for reloaded pointer publications used as later memory-address
  bases.
- Representative `pr57861.c` emitted an RV64 object after the `%lv.l`
  publication repair, proving the publication owner advanced past the prior
  `unsupported_local_memory_access` boundary.

Broader matched RV64 regression guard passed using canonical logs for:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "riscv64"
```

The before log was captured at `50e1b8df1`; the after log was captured at
current `main`. The supervisor guard command was:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Guard result: passed. Counts were before passed=89 failed=18 total=107, after
passed=92 failed=18 total=110, delta passed=+3 failed=0, new failing tests=0.

Based on focused proof, representative proof, the repaired local-string
compound regression, and the non-regressing matched RV64 guard, idea 649 appears
ready for reviewer or lifecycle close consideration.

## Suggested Next

Proceed to reviewer or plan-owner close gate for idea 649. The executor-side
evidence supports close, subject to the lifecycle owner validating the source
idea completion criteria.

## Watchouts

- Do not treat scalar frame-slot local-memory facts from idea 640 as
  pointer/global local-publication authority.
- Do not reopen direct global-symbol local-memory support from idea 631 or
  generic pointer freshness from idea 600 unless fresh evidence proves a new
  local-publication boundary.
- Do not use the `main` call-argument direct-global select-chain evidence as
  the owner for this idea; the representative owner is inside `foo` around
  `%lv.l`.
- Keep named `StoreLocalInst` lookup exact for pointer/global publication; only
  anonymous same-instruction accesses with no prepared value identity were
  restored for existing compound local-memory support.
- Keep the live-load expected-failure coverage intact; arbitrary reloaded
  pointer publications are not supported.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

No new proof command was run for this recording-only packet. The accepted proof
is the canonical matched before/after RV64 regression guard using
`test_before.log` and `test_after.log`:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: passed with before passed=89 failed=18 total=107, after passed=92
failed=18 total=110, delta passed=+3 failed=0, and new failing tests=0.
