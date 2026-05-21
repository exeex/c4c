Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representatives And Classify Residuals

# Current Packet

## Just Finished

Step 4 ran the delegated `00204` representative proof and classified the next
residual after the fixed second-`stdarg` `%9s` byval aggregate publication.
No implementation, test, expectation, runner, `plan.md`, or source-idea files
were changed.

The proof confirms the prior `%9s` first bad fact is gone: the second
`stdarg` line now prints `lmnopqr ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI
ABCDEFGHI`. The next concrete first bad fact is immediately after that line in
`HFA long double:`. The first HFA payload call is source
`myprintf("%hfa34 %hfa34 %hfa34 %hfa34", hfa34, hfa34, hfa34, hfa34)`;
expected output starts with `34.1,34.4 34.1,34.4 ...`, but runtime output
starts with `0.0,0.0 0.0,0.0 0.0,0.0` and then huge long-double values before
later `HFA double:` / `HFA float:` corruption.

Generated AArch64 evidence localizes the owner to the variadic HFA
FP-register-save-area path. The `.str52` callsite loads the first two `hfa34`
arguments into `q0`-`q7` and spills the later two 64-byte HFA payloads into
the outgoing overflow area. In `myprintf`, the `%hfa34` `va_arg` branch uses
the aggregate HFA access plan: it reads from the FP register-save-area while
`fp_offset + 64 <= 0`, reconstructs four q-lanes from 16-byte slots, advances
`fp_offset` by 64, and then publishes lanes `a` and `d` to
`printf("%.1Lf,%.1Lf", ...)`. That path should materialize the first
register-resident `hfa34` from saved `q0`-`q3`, but the first printed pair is
already `0.0,0.0`.

AST-backed lookup confirmed the relevant planning boundary is
`infer_aapcs64_hfa_va_arg_shape` feeding
`make_aapcs64_aggregate_va_arg_access_plan` in
`src/backend/prealloc/variadic_entry_plans.cpp`; the generated branch matches
that HFA register-save-area plan rather than the ordinary GPR/byval aggregate
plan.

## Suggested Next

Repair or instrument the general AArch64 variadic HFA register-save-area
publication/materialization path for anonymous HFA aggregate `va_arg`, starting
with a focused backend case for `struct { long double a,b,c,d; }` passed to a
variadic callee and read by `va_arg`.

## Watchouts

This is not the fixed non-HFA `%9s` byval aggregate GPR-lane publication
fault: `.str50` now reaches all six payloads. It is also not the earlier
HFA return-value lane, fixed-formal entry publication, stdarg cursor
progression for `%7s` / `%9s`, MOVI zero-extension, expectation, runner, or
unsupported-classification owner.

Classification: continue inside idea 326. The residual is specifically an
AArch64 variadic HFA/floating aggregate call-boundary and `va_arg`
register-save-area source/publication issue. A lifecycle handoff is not
needed unless a focused probe proves the first `hfa34` payload arrives intact
in `myprintf`'s saved `q0`-`q3` and is only corrupted by the nested libc
`printf` call, which current generated evidence does not show.

## Proof

Ran the delegated Step 4 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$' > test_after.log 2>&1
```

Result: build succeeded/up to date; the selected `00204` CTest still failed
with `RUNTIME_NONZERO` / segmentation fault after advancing past the fixed
`%9s` byval aggregate line. `test_after.log` is the preserved proof log and
contains the runtime output used for this classification.
