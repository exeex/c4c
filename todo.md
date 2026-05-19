Status: Active
Source Idea Path: ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Mismatch Mechanism

# Current Packet

## Just Finished

Lifecycle switch: parked idea 304 after its timeout-specific repair and
activated idea 305 for the now-reachable `00205` output mismatch.

## Suggested Next

Delegate Step 1 to classify the `00205` value-materialization or stack-frame
mechanism. Start from generated assembly reads such as `[sp, #632]`,
`[sp, #1064]`, and `[sp, #1496]` despite the `sub sp, sp, #48` prologue.

## Watchouts

- Preserve the idea 304 timeout repair: `00205` should complete quickly and
  must not return to the 5-second timeout path.
- Preserve the idea 303 legality repair: generated assembly must keep legal
  sign-extension spelling such as `sxtw x9, w13`.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, proof-log policy, or CTest registration.
- Do not special-case filename `00205`, exact stack offsets, case indexes, or
  temporary labels.

## Proof

No new proof was run for this lifecycle-only switch. The accepted current
baseline is the post-timeout state in `test_before.log`: `00064` and `00139`
pass; `00205` completes quickly and fails output comparison.
