Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair General Call-Boundary Preparation

# Current Packet

## Just Finished

Step 3 attempted the delegated HFA register-save-area repair packet but stopped
blocked after fresh generated-code and debugger evidence contradicted the
packet's first-bad classification.

AST-backed lookup confirmed the named HFA planning path:
`infer_aapcs64_hfa_va_arg_shape` feeds
`make_aapcs64_aggregate_va_arg_access_plan`, and the AArch64 printer emits
the aggregate `va_arg` register-save-area branch from
`src/backend/mir/aarch64/codegen/variadic.cpp`.

The fresh proof still has `backend_.*` and `00140` green, but `00204` does not
cleanly reach the later standalone
`myprintf("%hfa34 %hfa34 %hfa34 %hfa34", ...)` call. The generated
`myprintf` loop for the second `stdarg` `%7s %9s ...` invocation stores the
current format byte with `strb` and then reloads it as an 8-byte value from the
same stack address before comparing against zero. That stale-wide reload keeps
the loop alive past the string terminator, so runtime output walks through the
adjacent `.str51` / `.str52` literals and prints NUL bytes plus the visible
`HFA long double:` text inside the same invocation.

Speculative HFA changes were removed. No implementation change is left in the
tree because the current blocking fact is the format-byte materialization/NUL
test path, not a proven HFA register-save-area copy defect.

## Suggested Next

Classify or repair the AArch64 byte load / NUL-test materialization that lets
`myprintf`'s `for (s = format; *s; s++)` loop read past the terminator. Resume
the HFA register-save-area packet only after the standalone `%hfa34`
invocation is actually reached as a first bad fact.

## Watchouts

Do not commit a HFA register-save-area change from this packet: the attempted
payload-home and q-lane copy probes did not advance `00204` and were reverted.

The visible `HFA long double:` output in `test_after.log` is currently
ambiguous because debugger evidence shows it can be reached by overreading
adjacent string literals after the second `%9s` format, rather than by entering
the later source-level HFA call.

## Proof

Ran the delegated Step 3 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded and CTest was 144/145 green. All selected
`backend_.*` tests passed and `c_testsuite_aarch64_backend_src_00140_c`
passed. The only failure was
`c_testsuite_aarch64_backend_src_00204_c`, still `RUNTIME_NONZERO` /
segmentation fault. `test_after.log` is the preserved proof log.
