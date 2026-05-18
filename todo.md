# AArch64 String/Global Address External Call Lowering Todo

Status: Active
Source Idea Path: ideas/open/287_aarch64_string_global_address_external_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Expand to Starter Stdio/Data Representatives

# Current Packet

## Just Finished

Step 3 (`Expand to Starter Stdio/Data Representatives`) ran the selected
starter subset for `src/00125.c`, `src/00131.c`, `src/00154.c`, `src/00161.c`,
and `src/00211.c`.

The address-materialization/direct-call repair generalized across the starter
stdio/data representatives that exercise this owner: `00125`, `00131`,
`00154`, and `00211` passed in the delegated AArch64 backend subset. The
remaining `00161` failure is separated from this source idea: the generated
assembly materializes the format string and moves the formatted scalar argument
before `bl printf`, but then omits the scalar `a = t + p` update after the call.
Prepared BIR contains `%t6 = bir.add i32 %t4, %t5` followed by the store to
`%lv.a`; emitted AArch64 loads `t` and `p` and then stores `t` back to `p`
without emitting the add/store-result sequence for `a`.

## Suggested Next

Treat Step 3 as blocked only by the separated `00161` scalar/local-state issue.
The next coherent packet for this source idea is Step 4 sampling of nearby
stdio/data mismatch cases after the supervisor decides whether to route the
`00161` scalar stack-slot result/local update gap into a separate focused idea.

## Watchouts

- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, CTest registration, parser, or sema.
- Reject named-case shortcuts, literal-spelling shortcuts, and `printf`-only
  special cases.
- `src/00161.c` should not be repaired inside this address/direct-call route
  unless the proposed change is a semantic AArch64 scalar/local-store repair.
  Its call sequence is already correctly lowering the string/global direct-call
  argument owner.
- Keep `src/00132.c` out of first proof; it remains timeout-sensitive and
  loop/local-store-compounded.
- Keep broad aggregate ABI/HFA returns, broad variadic ABI completeness,
  `_Generic`, wide chars, and function-pointer casts out of this route.

## Proof

Ran the supervisor-selected proof:

```bash
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00(125|131|154|161|211)_c$'; } > test_after.log 2>&1
```

Exit code was `8`: `00125`, `00131`, `00154`, and `00211` passed; `00161`
failed with a runtime mismatch, printing only `1` instead of the Fibonacci
sequence through `89`. Canonical proof log: `test_after.log`.
