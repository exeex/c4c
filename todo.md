# AArch64 Scalar Call Value Semantics Todo

Status: Active
Source Idea Path: ideas/open/286_aarch64_scalar_call_value_semantics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Scalar Call-Value Failure Shape

# Current Packet

## Just Finished

Step 1, "Confirm Scalar Call-Value Failure Shape", completed as an
investigation packet. `00116.c` exposes an ordinary scalar direct-call argument
mismatch: prepared BIR records `f(0)` as `arg0 ... to=x0`, but current AArch64
assembly emits `bl f` without materializing `0` into `w0`/`x0`. `00159.c`
shows the same direct-call argument gap for `myfunc(3)`, `myfunc(4)`, and
`vfunc(1234)`: prepared call plans bind each immediate argument to `x0`, while
the emitted call sites branch directly. The prepared call-result facts bind
returns from `x0` into caller homes (`x13` in these probes), and the emitted
after-call move currently appears as `mov x13, x0`.

Likely owning surfaces are `src/backend/mir/aarch64/codegen/calls.cpp` for
`BeforeCall` immediate-to-ABI argument moves and call-boundary move printing,
`src/backend/mir/aarch64/codegen/returns.cpp` plus
`src/backend/mir/aarch64/codegen/machine_printer.cpp` for named scalar return
production through `BeforeReturn`/`x0`, and the prepared ABI policy helpers in
`src/backend/prealloc/target_register_profile.cpp` as read-only authority for
`x0` argument/result placement. The current failure is not owned by variadic
`printf` or string lowering; those only add later noise in `00159.c`.

## Suggested Next

First code slice should add focused backend coverage for ordinary AArch64
scalar direct-call immediate arguments and named scalar returns, then repair
the call-boundary/return-boundary lowering against those contracts.

Recommended first proof command:

```sh
cmake --build build --target backend_aarch64_machine_printer_test backend_aarch64_return_lowering_test && ctest --test-dir build -R 'backend_aarch64_(machine_printer|return_lowering)' --output-on-failure && ./build/c4cll --target aarch64-unknown-linux-gnu --codegen asm tests/c/external/c-testsuite/src/00116.c -o /tmp/c4c_00116_aarch64.s && sed -n '1,80p' /tmp/c4c_00116_aarch64.s
```

## Watchouts

- Keep this route on ordinary scalar direct-call values first.
- `00116.c` is the cleanest first proof because it isolates one direct call,
  one scalar argument, and one scalar return.
- `00159.c` confirms the same immediate-argument hole for ordinary same-module
  calls, but do not use its `printf` strings or void-call side behavior as the
  first owner.
- The current prepared facts already name AAPCS64 `x0` for scalar argument and
  result positions; the first implementation slice should consume those facts
  instead of inventing local ABI policy.
- Do not absorb variadic `printf`, string-literal addressing, loop predicates,
  short-circuit control flow, aggregate/pointer lowering, static storage, goto,
  or macro/preprocessor-adjacent cases.
- `00116.c` and `00159.c` are probes, not implementation selectors.
- Do not turn remaining failures into expectation changes, allowlist edits,
  timeout changes, CTest runner changes, or c-testsuite-specific shortcuts.

## Proof

Investigation-only packet; no implementation or test files changed and no
acceptance build was required. Inspection commands run:

```sh
./build/c4cll --target aarch64-unknown-linux-gnu --codegen asm tests/c/external/c-testsuite/src/00116.c -o /tmp/c4c_00116_aarch64.s
./build/c4cll --target aarch64-unknown-linux-gnu --codegen asm tests/c/external/c-testsuite/src/00159.c -o /tmp/c4c_00159_aarch64.s
./build/c4cll --target aarch64-unknown-linux-gnu --dump-prepared-bir --mir-focus-function main tests/c/external/c-testsuite/src/00116.c
./build/c4cll --target aarch64-unknown-linux-gnu --dump-prepared-bir --mir-focus-function main tests/c/external/c-testsuite/src/00159.c
```

No `test_after.log` was refreshed for this investigation packet.
