# AArch64 String/Global Address External Call Lowering Todo

Status: Active
Source Idea Path: ideas/open/287_aarch64_string_global_address_external_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Static/Global Data Pointer Materialization

# Current Packet

## Just Finished

Step 4 (`Sample Related Mismatch Cases and Decide Closure`) ran the selected
related stdio/data subset excluding the previously separated `00161` scalar
local-state case and the timeout-sensitive `00132` overlap case.

Classification:

- Passed: `00125`, `00131`, `00154`, `00166`, `00184`, `00190`, `00191`,
  `00201`, and `00211`.
- Runtime mismatch with no output: `00156`, `00158`, `00160`, `00168`,
  `00169`, and `00183`.
- Runtime mismatch with partial or wrong output: `00159`, `00192`, and
  `00197`.
- Runtime nonzero: `00206` segfaulted.
- Timed out: none.

Owner split:

- Still visible in this source-idea family: `00197` exercises static/global
  data value materialization and prints address-shaped negative values instead
  of `fred`/`joe` values; `00206` exercises string-literal pointer arguments to
  `printf("%s")` and segfaults before producing output.
- Separate owners, do not broaden this plan around them: `00156`, `00158`,
  `00160`, `00168`, `00169`, `00183`, and `00192` are dominated by loop,
  branch, switch, conditional-expression, recursion, or scalar/local-state
  behavior after the starter external-call string/global cases already pass.
  `00159` is dominated by internal direct-call argument/return behavior and
  should not be treated as the external `printf` string/global address owner.

## Suggested Next

Continue the active runbook. Step 5 now owns `00197` as a focused
static/global data materialization packet. Step 6 now owns `00206` as a
focused `%s` string-literal direct external-call pointer packet.

Suggested Step 5 proof should start narrow on `src/00197.c`, then include the
already-green starter owner subset when practical. Do not create follow-on
ideas for the other sampled failures from this lifecycle action; they remain
separate owners outside this active source idea until the supervisor chooses to
open them explicitly.

## Watchouts

- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, CTest registration, parser, or sema.
- Reject named-case shortcuts, literal-spelling shortcuts, and `printf`-only
  special cases.
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
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure --timeout 5 -R '^c_testsuite_aarch64_backend_src_00(125|131|154|156|158|159|160|166|168|169|183|184|190|191|192|197|201|206|211)_c$'; } > test_after.log 2>&1
```

Exit code was `8`: 9 tests passed and 10 failed. Nonzero result is classified
above and allowed for this sampling packet. Canonical proof log:
`test_after.log`.
