# AArch64 String/Global Address External Call Lowering Todo

Status: Active
Source Idea Path: ideas/open/287_aarch64_string_global_address_external_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair %s String-Literal External-Call Pointer Arguments

# Current Packet

## Just Finished

Step 6 (`Repair %s String-Literal External-Call Pointer Arguments`) repaired
the AArch64 prepared direct-call path for string/global address arguments.
Call dispatch now emits every prepared address materialization attached to a
call instruction instead of only the first one, and stack-home symbol/string
call arguments can materialize directly into their ABI argument register when
there is no stored pointer value to reload.

`src/00206.c` now passes on the AArch64 backend route: each `printf("%s", ...)`
call receives the correct string literal address in `x1` instead of using a
missing or stale pointer argument.

## Suggested Next

Supervisor should review this Step 6 slice for commit readiness and decide
whether the active runbook is ready for owner-side closure.

## Watchouts

- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, CTest registration, parser, or sema.
- Reject named-case shortcuts, literal-spelling shortcuts, and `printf`-only
  special cases.
- This packet fixes direct external-call pointer arguments whose address
  materialization is already present in prepared addressing. It does not widen
  aggregate globals, pointer-initializer globals, GOT/TLS storage models, or
  broader variadic ABI coverage.
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
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00206_c$'; } > test_after.log 2>&1
```

The command exited `0`, and `c_testsuite_aarch64_backend_src_00206_c` passed.
Canonical Step 6 proof log: `test_after.log`.

Supervisor acceptance proof also passed:

```bash
ctest --test-dir build -j --output-on-failure -R '^backend_'
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00(125|131|154|197|206|211)_c$'; } > /tmp/c4c_step6_owner_subset.log 2>&1
cmake --build --preset default
```

The backend subset passed 139/139, the Step 6 owner subset passed 6/6, and
the default preset build passed. Owner subset proof log:
`/tmp/c4c_step6_owner_subset.log`.
