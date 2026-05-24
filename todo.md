Status: Active
Source Idea Path: ideas/open/call-boundary-move-classification-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Adapt AArch64 To Consume The Shared Classification

# Current Packet

## Just Finished

Step 3 adapted AArch64 before-call and after-call move resolution to consume
`prepare::classify_prepared_call_boundary_move(...)` for the shared
call-boundary classification surface.

The AArch64-local argument/result plan and ABI binding lookup helpers were
removed from `calls_argument_sources.cpp`/`calls.hpp`; `calls_moves.cpp` now
uses the shared classification's matched Prepared argument plan, result plan,
and ABI binding pointers. AArch64 still owns register conversion/spelling, MIR
operand construction, diagnostics, byval/f128/sret policy, preservation
emission, printed records, and final instruction emission.

The shared helper was kept target-neutral while preserving matched ABI binding
authority for missing or mismatched call-result plans. This preserves existing
AArch64 HFA lane fallback behavior without reintroducing target-local raw
binding decoding.

## Suggested Next

Step 4 should prove the reuse path for x86 prepared operands by exposing or
using the same prealloc call-boundary classification helper from the x86
prepared surface, without rewriting x86 lowering or moving x86-specific
operand spelling/encoding into prealloc.

## Watchouts

AArch64 now consumes only the narrow move-to-plan and move-to-binding facts.
Preservation lookup/emission remains target-local and was not extracted.

Keep the classification result as facts/status only in future consumers; do
not move target register spelling, MIR operands, diagnostics, or instruction
encoding into prealloc.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 153/153 backend tests. Proof log: `test_after.log`.
