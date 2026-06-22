# RV64 Text-Only Fail-Closed Output Contract

## Goal

Repair the RV64 prepared-assembly output contract so a prepared function that
contains `main` either emits a real RV64 function symbol and body or fails with
an explicit unsupported diagnostic before producing a linkable-looking
`.text`-only assembly file.

## Why This Exists

The RV64 c-testsuite undefined-main triage classified all 93 linker
`undefined reference to main` cases under `rv64_text_only_fail_closed`.
Prepared BIR still contains `prepared.func @main`, but the final assembly file
contains only:

```asm
    .text
```

The minimal control is `src/00094.c`:

```c
extern int x;
int main() { return 0; }
```

Its prepared BIR has `main`, no storage values, no calls, and no addressing
demand. That means the first repair must prove the function emission contract
without depending on string literals, aggregate globals, pointer globals,
floating globals, or libc calls.

Evidence to reuse:

- `build/rv64_c_testsuite_probe_v2/classification.md`
- `build/rv64_c_testsuite_probe_v2/representative_evidence.md`
- `build/rv64_c_testsuite_probe_v2/repair_order.md`
- `build/rv64_c_testsuite_probe_v2/classification_work/buckets/rv64_text_only_fail_closed.txt`
- `build/rv64_c_testsuite_probe_v2/classification_work/buckets/unused_extern_no_storage.txt`

## In Scope

- Identify why the RV64 prepared module route returns successful `.text`-only
  output even when prepared BIR contains an emit-ready function.
- Ensure `src/00094.c` emits `.globl main`, a `main:` label, and an executable
  function body for RV64.
- Ensure unsupported global/module features do not silently erase otherwise
  prepared functions behind a successful assembly output.
- Add focused backend coverage for the minimal output-contract behavior.
- Keep the full 93-case list as triage evidence, not as a required CTest sweep
  for this idea.

## Out Of Scope

- Implementing string literal storage or external call ABI support.
- Implementing aggregate, pointer, floating, or broad scalar global storage.
- Making every case in the 93-case undefined-main bucket pass at link or qemu
  runtime.
- Weakening existing backend contracts to accept empty `.text` as a supported
  output.

## Candidate Cases

- Minimal control: `src/00094.c`
- Primary representative hazards after the control is fixed:
  - `src/00024.c` for aggregate global storage
  - `src/00025.c` for string literal address materialization plus `strlen`
  - `src/00045.c` for pointer global storage
  - `src/00119.c` for floating global storage
- Full affected bucket: the 93 cases listed in
  `build/rv64_c_testsuite_probe_v2/classification_work/buckets/rv64_text_only_fail_closed.txt`

## Acceptance Criteria

- RV64 assembly generation for `src/00094.c` no longer produces `.text` only;
  it contains `.globl main`, `main:`, and a real return path.
- If a module contains unsupported global storage or address materialization,
  the backend does not claim success with empty function output. It either
  emits supported functions correctly or reports a clear unsupported condition.
- A focused regression test covers the no-storage prepared `main` case and
  would fail on the old `.text`-only behavior.
- Representative secondary cases are rechecked narrowly after the fix so their
  next failure mode is visible instead of hidden behind undefined `main`.
- No implementation claims that all 93 cases are feature-complete merely
  because `main` now emits.

## Reviewer Reject Signals

- The patch special-cases `src/00094.c`, `main`, or an exact c-testsuite source
  shape instead of repairing the RV64 prepared-function emission contract.
- Empty `.text` output remains accepted as a successful assembly result for a
  prepared module that contains an emit-selected function.
- The diff downgrades supported-path expectations, marks the control case
  unsupported, or weakens tests without explicit user approval.
- The route claims c-testsuite progress by rewriting expected diagnostics,
  renaming helpers, or changing classification artifacts without repairing
  function emission.
- The patch folds string literals, aggregate globals, pointer globals, or
  floating globals into this idea and obscures whether the no-storage control
  works independently.
- The exact old failure mode remains, but is hidden behind a new abstraction
  name or a later linker failure.
