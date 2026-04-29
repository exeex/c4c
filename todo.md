Status: Active
Source Idea Path: ideas/open/126_bir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prefer Semantic Identity In Function, Call, And Global Paths

# Current Packet

## Just Finished

Step 3 completed for the string-pool direct-call rewrite bridge in
`src/backend/bir/lir_to_bir.cpp`.

`rewrite_direct_call_string_pointer_args` now pairs lowered BIR functions with
their LIR source functions by `Function::link_name_id` /
`LirFunction::link_name_id` when the lowered function has a semantic id. Raw
function-name fallback is restricted to functions where the lowered BIR
function and candidate LIR function both lack semantic ids.

Added backend coverage in `tests/backend/backend_lir_to_bir_notes_test.cpp`
with a raw-name decoy LIR function. The focused fixture proves the string-pool
rewrite uses the semantic function id for the bridge and preserves the final
`@.str.good` string-pool spelling instead of taking aliases from the raw-name
decoy.

## Suggested Next

Continue Step 3 on the next function, call, or global path where raw string
lookup still controls semantic identity and a `LinkNameId` carrier is already
available.

## Watchouts

The direct-call rewrite still aligns LIR calls to lowered BIR calls by call
order inside the matched function. This packet only removed raw function-name
authority from the LIR/BIR function pairing. String-pool constants still
intentionally use local `TextId` identity and final retained spelling, not
`LinkNameId`.

## Proof

Passed delegated Step 3 proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: build completed; `ctest` reported 100% tests passed, 0 failed out of
108 run in the `^backend_` subset, with 12 disabled tests not run.
Proof log: `test_after.log`.
