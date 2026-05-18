# AArch64 String/Global Address External Call Lowering Todo

Status: Active
Source Idea Path: ideas/open/287_aarch64_string_global_address_external_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Static/Global Data Pointer Materialization

# Current Packet

## Just Finished

Step 5 (`Repair Static/Global Data Pointer Materialization`) repaired the
AArch64 prepared-memory route for scalar static/global objects. The generated
assembly for `src/00197.c` now emits scalar globals in `.data`, lowers
`bir.load_global` into symbol-address materialization plus `ldr`, and lowers
`bir.store_global` into symbol-address materialization plus `str`.

Follow-up: restored the exported
`make_prepared_frame_slot_load_memory_instruction_record(...)` local-load API
as a compatibility wrapper around the generalized load helper so the default
build tests continue to link.

Follow-up: updated the focused backend contracts for the newly supported scalar
symbol-backed memory path. Scalar global-symbol load/store now selects, while
missing symbol identity and unsupported string-constant memory bases still
fail closed.

`src/00197.c` now prints the expected `fred`, `joe`, and static-local values
instead of address-shaped values for this source-idea owner.

## Suggested Next

Continue to Step 6 as a focused `%s` string-literal direct external-call
pointer packet for `src/00206.c`.

## Watchouts

- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, CTest registration, parser, or sema.
- Reject named-case shortcuts, literal-spelling shortcuts, and `printf`-only
  special cases.
- Step 5 only covered scalar static/global object emission plus symbol-backed
  memory load/store for the prepared AArch64 route. Aggregate globals,
  pointer-initializer globals, GOT/TLS storage models, and wider global data
  emission remain outside this packet unless the supervisor explicitly scopes
  them.
- `src/00161.c` remains separated as a scalar/local-state issue; its call
  sequence was already correctly lowering the string/global direct-call
  argument owner.
- `src/00132.c` remains intentionally excluded here because it is
  timeout-sensitive overlap evidence.
- `00197` and `00206` are the only sampled failures that keep the
  string/global address materialization/direct external-call owner visible;
  they now have explicit active runbook steps.
- Keep broad aggregate ABI/HFA returns, broad variadic ABI completeness,
  `_Generic`, wide chars, and function-pointer casts out of this route.

## Proof

Ran the supervisor-selected proof:

```bash
ctest --test-dir build -j --output-on-failure -R '^backend_'
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00197_c$'; } > test_after.log 2>&1
```

Both commands exited `0`: the backend bucket passed, and
`c_testsuite_aarch64_backend_src_00197_c` passed. Canonical Step 5 proof log:
`test_after.log`.
