# AArch64 String/Global Address External Call Lowering

Status: Open
Created: 2026-05-18
Source Inventory: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Intent

Repair the AArch64 backend lowering/emission path that currently passes
uninitialized or incorrect values where string-literal, global/static data, or
other pointer-valued arguments should be materialized for direct external calls.

## Why This Exists

The refreshed AArch64 backend c-testsuite inventory selected a runtime-visible
family around `printf`/stdio calls. The smallest representative,
`src/00125.c`, only prints `"hello world\n"`, but generated assembly passes an
uninitialized-looking register value to `printf` instead of a materialized
address for the string literal. Nearby cases repeat the same owner with
formatted output, looped calls, struct field formatting, and macro integer
expressions:

- `src/00125.c`
- `src/00131.c`
- `src/00154.c`
- `src/00161.c`
- `src/00211.c`

This is backend capability work. It is not a runner, allowlist, expectation,
parser, sema, or timeout-policy problem.

## In Scope

- AArch64 backend lowering/emission for string-literal addresses.
- AArch64 backend lowering/emission for global/static data addresses when they
  flow as pointer values.
- Pointer-valued call arguments placed into the proper AArch64 ABI argument
  registers for direct external calls.
- Direct external-call argument setup where the broken value is an address or
  a formatted scalar argument flowing to `printf`/stdio.
- Focused proof using non-timeout starter cases before expanding to related
  stdio/data mismatch cases.

## Out of Scope

- Runner behavior, CTest registration, allowlists, unsupported classifications,
  expected-output rewrites, timeout settings, or test weakening.
- Parser or sema changes.
- Named-case shortcuts, filename checks, exact string-spelling shortcuts, or
  special-casing only `printf`.
- Full aggregate ABI or HFA return support.
- Broad variadic ABI completeness beyond ordinary integer/pointer argument
  placement needed by this family.
- `_Generic`, wide characters, function-pointer casts, and unrelated frontend
  feature admission.
- Using timeout case `src/00132.c` as the first proof; it may be supporting
  overlap evidence only after the basic family is repaired.

## Acceptance Criteria

- `src/00125.c` demonstrates correct AArch64 materialization of the string
  literal address and correct direct external-call argument placement.
- The repair generalizes across at least the starter family:
  `src/00125.c`, `src/00131.c`, `src/00154.c`, `src/00161.c`, and
  `src/00211.c`, subject to unrelated pre-existing blockers being documented.
- Related stdio/data mismatch cases are sampled after the first starter proof
  to show the route is not named-case-only.
- No pass-count improvement is claimed through expectation rewrites,
  allowlists, unsupported classifications, runner changes, or timeout policy.
- Any remaining timeout behavior, especially `src/00132.c`, is explicitly
  kept separate unless later evidence shows only this address/call owner
  remains.

## Reviewer Reject Signals

Reject the route if it:

- fixes only `src/00125.c`, one literal spelling, one generated register, or
  one c-testsuite filename instead of adding a semantic address
  materialization and call-argument lowering rule;
- special-cases `printf` while leaving the same pointer-valued direct
  external-call argument path broken for nearby stdio calls;
- claims backend progress through expected-output rewrites, allowlist changes,
  unsupported classifications, runner changes, timeout changes, or CTest
  contract changes;
- broadens into parser/sema work, aggregate ABI/HFA returns, broad variadic ABI
  completeness, `_Generic`, wide-char handling, function-pointer casts, or
  timeout-case repair before the starter family is proven;
- uses `src/00132.c` as the first acceptance proof or treats its timeout
  behavior as ordinary mismatch work;
- renames helpers or moves emission code while preserving the old failure mode
  where string/global addresses are not materialized into ABI argument
  registers before external calls.
