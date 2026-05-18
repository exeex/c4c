# AArch64 Scalar Call Value Semantics Todo

Status: Active
Source Idea Path: ideas/open/286_aarch64_scalar_call_value_semantics.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Runtime Call Probes

# Current Packet

## Just Finished

Step 4, "Prove Runtime Call Probes", completed for the delegated c-testsuite
runtime probes after the scalar immediate call-value repair. `00116.c` now
passes under CTest, so it no longer shows the old scalar immediate argument
materialization issue. `00159.c` also passes under the same runtime proof, so
there is no current evidence in this packet that it remains blocked by scalar
call-value lowering or by a separate `printf`/string scoped owner.

## Suggested Next

Next coherent packet should move to Step 5 and let the supervisor decide
whether to broaden validation for the scalar call-value slice or request
lifecycle review/closure handling.

## Watchouts

- Keep any follow-up validation on ordinary scalar direct-call values first.
- Do not absorb variadic `printf`, string-literal addressing, loop predicates,
  short-circuit control flow, aggregate/pointer lowering, static storage, goto,
  or macro/preprocessor-adjacent cases.
- `00116.c` and `00159.c` are probes, not implementation selectors.
- Do not turn remaining failures into expectation changes, allowlist edits,
  timeout changes, CTest runner changes, or c-testsuite-specific shortcuts.
- This packet did not inspect generated assembly because the delegated runtime
  proof was green for both probes.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
ctest --test-dir build --output-on-failure -R 'c_testsuite_src_001(16|59)_c' > test_after.log 2>&1
```

`test_after.log` shows `c_testsuite_src_00116_c` and
`c_testsuite_src_00159_c` both passing. The delegated proof was sufficient for
this Step 4 runtime probe packet.
