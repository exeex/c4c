Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representatives And Classify Residuals

# Current Packet

## Just Finished

Step 4 ran the delegated `00204` representative proof and classified the next
residual after the fixed HFA return-value lane. No implementation, test,
expectation, runner, `plan.md`, or source-idea files were changed.

The fixed `Return values:` block now reaches the expected HFA output through
`34.1 34.4`, including the previously bad `fr_hfa12().a` /
`fr_hfa12().b` lane (`12.1 12.2`).

The next concrete first bad fact is in `stdarg`, at the second variadic call:
source `myprintf("%7s %9s %9s %9s %9s %9s", s7, s9, s9, s9, s9, s9)`
should print `lmnopqr` followed by five `ABCDEFGHI` payloads. The fresh
runtime output prints only `lmnopqr ABCDEFGHI ABCDEFGHI` and then appends
`HFA long double:` without the expected remaining `%9s` payloads/newline.

Generated AArch64 evidence points to variadic aggregate call-boundary
publication, not to HFA return-value publication: the caller for `.str50`
loads the mixed `%7s` / `%9s` format into `x0`, publishes `s7` in `x1`, then
publishes only alternating register lanes for the following small aggregate
varargs (`x2`, `x4`, `x6`) while later chunks are written through the outgoing
overflow area. The missing `x3` / `x5` / `x7` aggregate lanes leave
`myprintf`'s register-save-area `va_arg` reads misaligned before the later HFA
long-double cases.

## Suggested Next

Repair the general AArch64 variadic aggregate call-boundary publication path
for small non-HFA aggregates that consume multiple GPR slots after a preceding
smaller aggregate vararg, starting from the `.str50` `%7s` + `%9s` evidence.

## Watchouts

This is not persistence of the fixed `Return values:` HFA lane. The fresh bad
fact occurs after that block, before the HFA long-double stdarg payloads are
decoded.

Classification: this remains inside the active runbook's reactivated
aggregate/varargs ABI call-boundary publication scope for idea 326 because the
evidence is a variadic call-boundary GPR-lane publication fault. It is not a
stdarg cursor/progression fault inside `myprintf`, not a fixed byval aggregate
call, and not an expectation/runner issue. It is also outside the narrower
HFA-return-value repair that just landed, so the next packet should be scoped
to variadic small-aggregate GPR lane publication rather than reopening floating
preserved-source retargeting.

## Proof

Ran the delegated Step 4 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$' > test_after.log 2>&1
```

Result: build succeeded/up to date; the selected `00204` CTest still failed
with `RUNTIME_NONZERO` / segmentation fault after advancing past the fixed
`Return values:` HFA block. `test_after.log` is the preserved proof log and
contains the runtime output used for this classification.
