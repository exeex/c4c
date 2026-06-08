# 128 AArch64 wide value owner post-contract audit

## Goal

Audit `src/backend/mir/aarch64/codegen/i128_ops.cpp` and
`src/backend/mir/aarch64/codegen/f128.cpp` after the recent BIR/prealloc,
call-boundary, carrier, aggregate transport, and memory-contract cleanup.

This idea is analysis-only. It should classify the remaining wide-value
lowering responsibilities and produce focused follow-up ideas only where there
is concrete evidence that shared BIR/prealloc facts should own behavior that is
still being rediscovered or duplicated in AArch64 codegen.

## Background

The AArch64 codegen layout is now much cleaner around calls, memory, ALU, and
comparison boundaries, but `i128_ops.cpp` and `f128.cpp` remain two large
special-width owners:

- `i128_ops.cpp` owns i128 pair operations, shifts, compares, runtime helper
  boundaries, transport record construction, and helper policy validation.
- `f128.cpp` owns f128 transport records, memory-backed carrier handling,
  runtime helper diagnostics, vector/Q-register spelling, and related
  helper-policy validation.

Earlier closure notes covered adjacent contracts:

- special carrier policy cleanup
- aggregate transport planning
- f128 call carrier ownership
- pointer-carrier provenance
- call argument/result publication
- memory-backed publication authority
- ALU post-contract audit

What is still missing is a post-contract pass over the wide-value owner files
themselves. The important question is not whether these files are large. The
question is whether they still contain shared lowering policy that should have
been prepared before AArch64 codegen, or whether their size is mostly target
local instruction emission and machine-record construction.

## Required analysis

Classify each major cluster in `i128_ops.cpp` and `f128.cpp` into one of these
owners:

1. Shared BIR/prealloc contract
   - target-neutral carrier choice
   - helper ABI/resource policy
   - preservation/publication facts
   - memory-backed transport authority
   - call-boundary wide-value facts
2. AArch64 codegen consumption
   - validation that prepared facts are complete and unambiguous
   - translation of prepared facts into AArch64 machine instructions
   - AArch64 register spelling, Q/X pair spelling, lane/shift opcode spelling
   - runtime helper call assembly after ownership is already selected
3. Local organization only
   - helper functions that are file-local and target-specific
   - printer or diagnostic formatting that does not decide lowering policy
   - candidate physical splits that do not change semantic ownership

The audit must explicitly inspect:

- i128 runtime helper ownership and div/rem ABI policy
- i128 preserved value and selected-call ownership checks
- i128 pair transport, shift, and compare record construction
- f128 full-width and memory-backed carrier facts
- f128 runtime helper resource/preservation checks
- f128 transport construction and printable-address handling
- overlap with `calls.cpp`, `memory.cpp`, `instruction.cpp`, and
  `machine_printer.cpp`

## Expected output

The closure note must contain:

- a short cluster map for `i128_ops.cpp`
- a short cluster map for `f128.cpp`
- a table of any remaining shared-policy rediscovery in AArch64 codegen
- a table of target-local code that should stay in AArch64
- zero or more follow-up ideas with precise filenames, proof routes, and reject
  signals

If no shared-policy gap remains, close with no follow-up ideas and explain why
the remaining size is target-local emission or local organization.

## Reject signals

- Rewriting or splitting the two files only because they are large.
- Moving AArch64 register spelling, Q-register spelling, lane/shift opcode
  spelling, or helper call assembly into shared BIR/prealloc code.
- Reopening closed carrier, transport, calls, or memory contracts without new
  evidence from these two files.
- Creating vague follow-up ideas such as "clean f128" or "shrink i128" without
  a concrete ownership boundary and proof route.
- Treating line count alone as evidence of architectural drift.
- Weakening tests or expectations to make the audit easier.

## Suggested proof route for follow-up ideas

This idea itself is analysis-only. Follow-up implementation ideas should choose
their own narrow proof, but likely candidates include:

- `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`
- targeted AArch64 backend internal tests covering i128/f128 transport and
  helper lowering
- a broader `^backend_` run when a shared BIR/prealloc contract changes
