Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Surface

# Current Packet

## Just Finished

Executed idea 295 Step 1 after idea 373 closure by running the backend-regex
surface command. The current backend-regex surface selected 356 tests: 349
passed, 7 failed, 2 of the 7 are timeouts.

All local backend/unit/route/MIR/BIR/CLI/runtime/smoke tests selected by
`-R backend` are green. The only residual failures are external AArch64
c-testsuite backend tests:
`00174`, `00187`, `00200` timeout, `00205`, `00207` timeout, `00216`, and
`00218`. The recently closed `00182` representative is no longer present in
the failing backend-regex surface.

## Suggested Next

Execute Step 2 classification for the 7 external residuals. Suggested first
focus remains the non-timeout semantic failures before the timeout bucket:
inspect `00174`, `00187`, `00205`, `00216`, and `00218`, while keeping
`00200`/`00207` parked as timeout representatives unless their generated code
gives a clear shared owner.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. The current surface is external-only; do
not spend Step 2 on local backend unit tests unless a new log contradicts this
run.

## Proof

Ran exact Step 1 command:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result: build completed; CTest returned nonzero for the expected residual
surface. `test_after.log` reports 356 selected, 349 passed, 7 failed, with
timeouts in `00200` and `00207`. Failed list:
`00174`, `00187`, `00200` timeout, `00205`, `00207` timeout, `00216`, `00218`.
