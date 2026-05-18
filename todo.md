# AArch64 Scalar Call Value Semantics Todo

Status: Active
Source Idea Path: ideas/open/286_aarch64_scalar_call_value_semantics.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Nearby Call Boundary

# Current Packet

## Just Finished

Step 5, "Reclassify Nearby Call Boundary", completed the delegated nearby
call-boundary runtime classification after the scalar call-value repair.

Pass: `00100.c`, `00116.c`, `00121.c`, `00125.c`, `00131.c`, `00132.c`,
`00154.c`, `00159.c`, `00161.c`, `00166.c`, `00172.c`, `00175.c`,
`00178.c`, `00184.c`, `00190.c`, `00191.c`, `00192.c`, `00196.c`,
`00197.c`, `00199.c`, `00201.c`, `00206.c`, and `00211.c`.

Fail: none.

Timeout: none.

The scalar call-value owner is exhausted for this delegated call-boundary
subset. The previously targeted scalar call probes `00116.c` and `00159.c`
remain in the fixed/pass bucket, and no remaining `printf`, string-literal,
variadic, loop, aggregate, global/static, short-circuit, or goto owner is
visible in this subset.

## Suggested Next

Next coherent packet should be plan-owner closure review for the scalar
call-value route, unless the supervisor wants broader validation before
lifecycle handling.

## Watchouts

- The delegated Step 5 subset is green under a 5-second CTest timeout, so this
  packet did not create any follow-on idea paths.
- Do not infer that unrelated `printf`, string-literal addressing, loop
  predicates, short-circuit control flow, aggregate/pointer lowering, static
  storage, goto, or macro/preprocessor-adjacent families are repaired outside
  this exact subset.
- Do not turn future boundary findings into expectation changes, allowlist
  edits, timeout changes, CTest runner changes, or c-testsuite-specific
  shortcuts.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
ctest --test-dir build --output-on-failure --timeout 5 -R 'c_testsuite_src_(00100|00116|00121|00125|00131|00132|00154|00159|00161|00166|00172|00175|00178|00184|00190|00191|00192|00196|00197|00199|00201|00206|00211)_c' > test_after.log 2>&1
```

`test_after.log` shows all 23 selected tests passed with 0 failures and 0
timeouts. Pre-run and post-run stale runtime/test process checks found only the
check commands themselves, so no process cleanup was needed. The delegated
proof was sufficient for this Step 5 classification packet.
