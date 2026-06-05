Status: Active
Source Idea Path: ideas/open/112_aarch64_00216_00204_post_closure_regression.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Shared Semantic Defect

# Current Packet

## Just Finished

Step 3 `Repair The Shared Semantic Defect` repaired the frame-slot call
argument address source-selection rule for direct local aggregate pointer
operands.

Semantic rule repaired: when a frame-slot-backed call argument is the direct
pointer operand being passed in a GPR, `call_plans.cpp` now treats that as
structured local-frame-address authority and allows the existing explicit
materialization lookup to match the aggregate lane/base name (`x` against
`x.0`). This keeps the `da0e90842` fail-closed requirement because selection
still requires a prepared frame-slot materialization when addressing facts are
available, and the old broad stack-layout compatibility path remains limited to
the no-addressing compatibility case.

Result: `c_testsuite_aarch64_backend_src_00216_c` now passes in the delegated
focused proof. The remaining `00204` failure is strictly narrower: it reaches
`stdarg`, completes the initial string aggregate variadic calls, prints
`HFA long double:`, then segfaults inside `myprintf` while fetching the first
stack-backed variadic HFA long-double aggregate via `va_arg`.

## Suggested Next

Execute a narrow follow-up on the remaining `00204` blocker: inspect AArch64
variadic HFA long-double call setup and `va_arg` consumption around the first
`myprintf("%hfa34 ...")` call in `stdarg`, focusing on register save area versus
overflow area authority for stack-backed variadic FPR/vector aggregate lanes.

## Watchouts

- Do not weaken, skip, or reclassify `00216` or `00204`.
- Do not add filename-shaped or expected-output-shaped special cases.
- The direct local aggregate pointer selection repair is intentionally bounded:
  it only broadens explicit materialization name matching for direct pointer
  operands with matching value/home identity and GPR destination.
- `00204` still has extensive variadic aggregate traffic, but the remaining
  crash now appears in HFA long-double variadic register-save/overflow handling,
  not in the direct local aggregate pointer address-selection path that fixed
  `00216`.
- Scratch evidence from this packet is under `/tmp`:
  `/tmp/c4c_00204_stdarg_after_step3.txt`,
  `/tmp/c4c_00204_arg_after_step3.txt`, and `/tmp/c4c_00204_after.s`.

## Proof

Ran delegated proof:

```sh
cmake --build --preset default && ctest --test-dir build -j1 --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00(216|204)_c)$' > test_after.log 2>&1
```

Result: build passed; focused CTest remains nonzero because
`c_testsuite_aarch64_backend_src_00204_c` still segfaults. The selected subset
is strictly narrower because `c_testsuite_aarch64_backend_src_00216_c` now
passes. Proof log path: `test_after.log`.
