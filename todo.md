Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representatives And Classify Residuals

# Current Packet

## Just Finished

Step 4 ran the delegated representative/backend proof after the AArch64
call-boundary authority repair.

The repaired call-boundary owner is stable under the selected local backend
coverage: every test selected by `backend_.*` passed in this proof subset.
The `00140` representative also passed, confirming the struct-plus-variadic
call-boundary crash from the activation evidence is repaired.

The old `00204` pre-runtime authority stop is gone. The representative now
assembles, links, runs, and fails as `RUNTIME_NONZERO` with a segmentation
fault after emitting corrupted HFA/floating output. The first concrete bad fact
is in the `Return values:` HFA-float block: expected `12.1 12.2` from
`printf("%.1f %.1f\n", fr_hfa12().a, fr_hfa12().b)`, but the runtime output is
`0.0 12.2`.

Generated AArch64 evidence points to HFA/floating lane publication, not the
old structured f128 call-boundary authority diagnostic. In `ret`, after
`bl fr_hfa12`, generated code stores `s0` and `s1`, reloads lane 0, converts
it into `d8`, reloads lane 1, converts it into `d9`, but then calls `printf`
with `fmov d0, d20` and `fmov d1, d9`. That makes the first printed lane come
from stale preserved-FPR state instead of the converted HFA return lane.

Classification: the remaining `00204` residual is still in idea 326 scope as
an HFA/floating value-to-call argument publication/materialization issue after
HFA return-value scalarization. It is a new first bad fact for the next
focused packet, not a lifecycle handoff target yet.

## Suggested Next

Localize and repair the HFA/floating return-lane-to-`printf` argument
publication owner represented by `00204`'s `Return values:` block, starting
from the stale `d20` publication after `fr_hfa12`.

## Watchouts

Do not treat the remaining `00204` failure as the old call-boundary authority
stop: assembly generation now succeeds, and the old diagnostic is absent.

The runtime also shows later HFA long-double, HFA double, HFA float, and
eventual segmentation-fault corruption, but the first bad fact is earlier in
the HFA return-value block. Classify or repair from that first fact before
chasing the later stdarg/HFA output.

No code, tests, expectations, unsupported classifications, runner behavior, or
CTest contracts were changed in this packet.

## Proof

Ran the delegated Step 4 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded and CTest was 144/145 green. All 143 selected
`backend_.*` tests passed, `c_testsuite_aarch64_backend_src_00140_c` passed,
and the only failure was `c_testsuite_aarch64_backend_src_00204_c`.

`00204` failed as `RUNTIME_NONZERO` with `exit=Segmentation fault`; the first
runtime mismatch recorded in `test_after.log` is `0.0 12.2` in the HFA
return-value block. `test_after.log` is the preserved proof log.
