# AArch64 Scalar Call Value Semantics Todo

Status: Active
Source Idea Path: ideas/open/286_aarch64_scalar_call_value_semantics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Scalar Call Value Lowering

# Current Packet

## Just Finished

Step 2, "Add Focused Scalar Call Tests", and Step 3, "Repair Scalar Call
Value Lowering", completed for ordinary scalar direct-call immediates and the
named scalar return boundary already covered by the focused return tests.
Focused AArch64 backend coverage now asserts that a scalar immediate call
argument prints as a selected call-boundary materialization into the AAPCS64
argument register, and that module lowering emits that materialization before
the direct call. The implementation consumes prepared call argument/binding
facts, not testcase names: binding-only `BeforeCall` scalar integer immediates
now lower to selected `CallBoundaryMove` nodes with a structured immediate
source and GPR ABI destination. `00116.c` assembly now emits `mov w0, #0`
immediately before `bl f`; the after-call scalar return consumption remains
`mov x13, x0`.

## Suggested Next

Next coherent packet should move to Step 4 and prove the runtime call probes,
starting with `00116.c` and then checking whether `00159.c` still has only the
previously scoped scalar call-value owner or exposes a separate owner.

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
- The delegated proof command does not relink `c4cll`; when codegen changes
  need the compiler binary, build `c4cll` before rerunning the exact proof
  command so the assembly probe uses the updated backend.

## Proof

Ran the supervisor-selected proof command exactly after relinking `c4cll` so
the assembly probe used the updated backend:

```sh
{ cmake --build build --target backend_aarch64_machine_printer_test backend_aarch64_return_lowering_test && ctest --test-dir build -R 'backend_aarch64_(machine_printer|return_lowering)' --output-on-failure && ./build/c4cll --target aarch64-unknown-linux-gnu --codegen asm tests/c/external/c-testsuite/src/00116.c -o /tmp/c4c_00116_aarch64.s && sed -n '1,80p' /tmp/c4c_00116_aarch64.s; } > test_after.log 2>&1
```

`test_after.log` shows both focused backend tests passing and the `00116.c`
assembly probe printing `mov w0, #0` before `bl f`.
