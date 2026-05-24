# Call Boundary Move Classification in Prealloc

## Intent

Extract generic Prepared call-boundary move classification from AArch64 call
dispatch into prealloc-owned call and move helper surfaces.

## Why This Exists

AArch64 codegen currently interprets prepared call plans, argument and result
sources, call moves, clobbers, and preservation records in code that is partly
generic and partly AAPCS64-specific. The reference codegen model keeps target
ABI emission in the backend, but generic call-boundary classification belongs
closer to prealloc and register-allocation call-move facts.

x86 will need the same call-boundary classification for prepared lowering.
RISC-V can use the same split later once it consumes Prepared facts.

## Current AArch64-Owned Responsibility

Current responsibility appears in generic portions of `dispatch_calls.cpp`,
`calls_common.cpp`, `calls_argument_sources.cpp`, `calls_moves.cpp`, and
`calls_preservation.cpp` that inspect Prepared call plans and move records
before AArch64-specific emission.

## Proposed Destination Layer

`src/backend/prealloc/calls.*`, `src/backend/prealloc/regalloc/call_moves.*`,
or a closely related prealloc call-boundary helper should own target-neutral
classification of call inputs, outputs, move bundles, clobbers, and
preservation requirements.

AArch64 should retain AAPCS64 sret and variadic details, concrete call record
printing, target register names, and instruction emission.

## x86/RISC-V Benefit

x86 prepared call lowering can reuse common argument/result, move, clobber, and
preservation classifications instead of copying AArch64 dispatch structure.
RISC-V benefits later because call-boundary facts become target-parameterized
records rather than backend-local rediscovery.

## In Scope

- Identify which current AArch64 call helper decisions are target-neutral
  Prepared call classifications.
- Add prealloc helper records for those classifications.
- Convert AArch64 to consume those helper records while keeping target ABI
  effects local.
- Name the x86 prepared-call lowering surface that would consume the same
  classification.
- Prove unchanged AArch64 call lowering for ordinary arguments/results,
  call-preservation moves, and at least one nontrivial clobber or move case.

## Out of Scope

- Rewriting AArch64 ABI policy, AAPCS64 sret, variadic lane handling, or printed
  call records.
- Moving final target call instruction emission into prealloc.
- Solving all call lowering gaps in one broad rewrite.
- Weakening call tests or replacing semantic proof with expectation edits.

## Proof Needed To Avoid Testcase Overfit

Acceptance needs focused proof over several call shapes, not only a single
failing call testcase. The set should include argument movement, result
publication, preservation/clobber handling where currently supported, and a
case that still exercises AArch64-specific ABI policy locally.

Reviewer should be able to see that target-specific details did not migrate
into the shared classification layer.

## Reviewer Reject Signals

Reject if the diff moves AAPCS64-specific sret, variadic, register spelling, or
printed call-record behavior into prealloc. Reject if the implementation adds
case-specific matching for one call signature, weakens unsupported expectations,
or claims progress by renaming AArch64 helper functions while classification
logic remains target-local.

Reject broad rewrites that combine call classification, operand decoding,
entry-formal publication, and call emission in one slice.
