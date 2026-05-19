# AArch64 Extern Data Symbol PIC Address Formation

Status: Open
Created: 2026-05-19

## Goal

Repair AArch64 address formation for externally binding data symbols so
`stdout`-like dynamic objects are accessed through a PIC-safe form instead of
direct local `adrp`/`:lo12:` relocation pairs that the linker rejects.

## Why This Exists

The fresh post-307 backend-regex inventory selected 352 tests, passed 306, and
failed 46. The crispest remaining singleton is
`c_testsuite_aarch64_backend_src_00189_c`, which fails during backend linking
with:

```text
R_AARCH64_ADR_PREL_PG_HI21 relocation against symbol `stdout@@GLIBC_2.17'
which may bind externally can not be used when making a shared object
```

Generated assembly for `00189.c` forms the `stdout` address directly:

```asm
adrp x10, stdout
add x10, x10, :lo12:stdout
ldr x9, [x10]
```

That is distinct from the closed symbol+offset/register-width and large scalar
immediate owners. The residual semantic owner is PIC-safe address materialization
for externally binding data symbols.

## In Scope

- Identify where the AArch64 backend decides whether a symbol reference is
  local/direct or externally binding/PIC-sensitive.
- Teach externally binding data-symbol address formation to use a linker-safe
  access sequence for dynamic symbols such as `stdout`.
- Preserve existing direct local symbol address formation for compiler-owned
  globals, string literals, and internal labels when legal.
- Cover the focused failing route
  `c_testsuite_aarch64_backend_src_00189_c`.
- Add or update narrow backend tests only when they assert the semantic symbol
  binding/addressing behavior rather than the exact c-testsuite filename.

## Out of Scope

- Runtime nonzero, runtime mismatch, crash, timeout, and output-storm buckets
  from the umbrella inventory.
- Machine-printer/prepared-node residuals `00140`, `00164`, and `00214`.
- Semantic `lir_to_bir` admission residuals `00204` and `00216`.
- Reopening closed owners 285 through 307 without new proof that contradicts
  their closure boundaries.
- Expectation, allowlist, unsupported-classification, CTest registration,
  runner, timeout-policy, or proof-log changes to improve counts.
- Filename-specific or instruction-string-specific shortcuts for `00189.c`.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00189_c` no longer fails with
  `R_AARCH64_ADR_PREL_PG_HI21` against `stdout@@GLIBC_2.17`.
- Generated AArch64 for externally binding data symbols uses a PIC-safe access
  sequence appropriate for dynamic data symbols.
- Existing local/internal symbol address formation remains legal and does not
  regress focused backend symbol-address tests.
- A fresh build or compile proof is recorded for the code slice.
- Supervisor-selected proof includes the focused c-testsuite target and enough
  adjacent symbol-address coverage to reject a named-test-only fix.

## Reviewer Reject Signals

Reject the route if it:

- fixes only `00189.c` by filename, C test shape, emitted instruction string,
  or hard-coded `stdout` handling instead of classifying symbol binding/address
  formation semantically;
- changes expectations, allowlists, unsupported classifications, CTest
  registration, runner behavior, timeout policy, or proof logs to claim
  progress;
- weakens existing local/internal symbol address behavior or routes all symbols
  through a broad fallback without preserving legal direct local forms;
- reopens closed symbol+offset width, large scalar immediate, or earlier
  backend owners without generated-code evidence that contradicts their
  closure boundary;
- claims capability progress from helper renames, classification-only changes,
  or diagnostics text changes while the linker can still emit
  `R_AARCH64_ADR_PREL_PG_HI21` against externally binding data symbols;
- broadens implementation into unrelated runtime, timeout, `lir_to_bir`, or
  machine-printer/prepared-node buckets parked under the umbrella inventory.
