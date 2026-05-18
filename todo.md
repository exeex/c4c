# AArch64 Function Pointer Indirect Call Values Todo

Status: Active
Source Idea Path: ideas/open/289_aarch64_function_pointer_indirect_call_values.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Local Function-Pointer Value Lowering

# Current Packet

## Just Finished

Completed Plan Step 2, "Repair Local Function-Pointer Value Lowering", for the
local function-pointer field route in `tests/c/external/c-testsuite/src/00087.c`.

Implemented the semantic native-backend repair by publishing prepared
address-materialization facts for indirect-call callee operands that already
carry structured function-symbol pointer identity. AArch64 global-address
lowering now honors matching explicit prepared home/storage register names
before falling back to placement-derived registers, so the materialized symbol
address reaches the same register consumed by `blr`.

Observed repaired `main` assembly materializes `foo` into the indirect callee
register and preserves the indirect call shape:

```asm
adrp x20, foo
add x20, x20, :lo12:foo
blr x20
```

Added focused backend coverage in `backend_prepare_stack_layout` for an
indirect function-symbol callee address materialization and in
`backend_aarch64_prepared_memory_operand_records` for explicit callee-register
name selection over placement fallback during address materialization.

## Suggested Next

Delegate Step 3 to expand the function-pointer value proof to global/static
function-pointer initialization and returned function-pointer representatives,
starting with `src/00089.c` and `src/00124.c`.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Reject named-case shortcuts, exact function-name shortcuts, aggregate-field
  shortcuts, and emitted-instruction-text matching.
- Do not treat this as residual stack-frame/SP alignment unless a new
  unaligned frame is proven.
- Preserve direct-call behavior and existing aligned frame behavior.
- Keep non-bus runtime failures from the wider sample outside this route unless
  they are proven to share the same function-pointer indirect-call owner.
- The Step 2 repair covers structured symbol-pointer indirect callees. Step 3
  should verify whether global initializers and returned function pointers also
  arrive at indirect calls with structured symbol identity, or whether they need
  a separate value-propagation repair.
- Avoid treating this as a direct-call conversion. The source expression is an
  indirect call through a function pointer field, and Step 2 should preserve
  that semantic shape instead of rewriting only this case to `bl foo`.

## Proof

Ran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00087_c$'; } > test_before.log 2>&1
```

Result: failed as expected; `test_before.log` records the Step 1 baseline
`[RUNTIME_NONZERO] .../src/00087.c exit=Bus error`.

Ran focused backend coverage:

```sh
cmake --build build-aarch64-scan --target backend_prepare_stack_layout_test backend_aarch64_prepared_memory_operand_records_test c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^(backend_prepare_stack_layout|backend_aarch64_prepared_memory_operand_records)$'
```

Result: passed.

Ran the delegated Step 2 proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00087_c$'; } > test_after.log 2>&1
```

Result: passed; `test_after.log` records
`c_testsuite_aarch64_backend_src_00087_c` passing.

Supervisor acceptance proof for Step 2:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: passed; the regression guard resolved
`c_testsuite_aarch64_backend_src_00087_c`.

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed 139/139.

```sh
cmake --build --preset default
```

Result: passed.

Follow-on owner probe:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00(087|089)_c$|^c_testsuite_aarch64_backend_src_00124_c$'; } > /tmp/c4c_fnptr_step2_owner_probe.log 2>&1
```

Result: selected 3 tests. `00087` passed, `00089` bus errored, and `00124`
segfaulted, confirming Step 3 still owns global/returned function-pointer
work.

Transient inspection artifacts used:
- `/tmp/00087.prepared-after`
- `/tmp/00087.after.s`
- `/tmp/00087.after2.s`
