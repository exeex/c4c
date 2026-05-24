# Shared Prepared Consumer Missing-Fact Diagnostics

## Intent

Create shared diagnostic builders for missing Prepared facts currently reported
from AArch64 dispatch.

## Why This Exists

AArch64 currently has diagnostic patterns for missing function context, value
authority, typed register authority, storage plan, call plan, block mapping,
and instruction mapping while consuming Prepared facts. These diagnostics are
not target instruction selection. If x86 and later RISC-V consume the same
Prepared facts, duplicating message construction and missing-fact categories in
each backend creates drift.

## Current AArch64-Owned Responsibility

Current responsibility is concentrated in `dispatch_diagnostics.cpp` and
related AArch64 module context code that formats missing Prepared-fact
diagnostics during lowering.

## Proposed Destination Layer

Use `src/backend/prealloc` for fact-owner-facing missing Prepared authority
diagnostics, or shared `src/backend/mir` support for machine-consumer context
diagnostics. The implementation should pick the narrower owner per diagnostic
category and avoid a broad diagnostics framework.

## x86/RISC-V Benefit

x86 prepared consumers can report the same missing Prepared authority classes
without duplicating AArch64 message construction. RISC-V benefits later by
getting standardized Prepared-consumer diagnostics as soon as it consumes the
same facts.

## In Scope

- Inventory AArch64 missing Prepared-fact diagnostic categories.
- Extract one small shared diagnostic-builder surface for categories that are
  clearly target-independent.
- Convert AArch64 to use the shared builders without changing behavior.
- Identify which x86 prepared-consumer path should use the same builders.
- Preserve or improve diagnostic specificity.

## Out of Scope

- Rewriting all backend diagnostics.
- Moving target-specific AArch64 instruction or ABI error messages.
- Changing failure classification to hide unsupported compiler paths.
- Combining diagnostics extraction with functional lowering changes.

## Proof Needed To Avoid Testcase Overfit

Acceptance needs before/after diagnostic proof for more than one missing-fact
category, preferably from existing targeted tests or direct narrow lowering
probes. The proof must show that messages remain specific enough to identify
the missing Prepared authority and that no expected supported path is
downgraded.

## Reviewer Reject Signals

Reject if the route weakens diagnostics into generic "lowering failed"
messages, reclassifies supported failures as unsupported, moves target-specific
AArch64 emission errors into shared Prepared diagnostics, or claims capability
progress from message-only changes.

Reject if only one narrow named missing-fact testcase is handled while nearby
missing authority categories still duplicate AArch64-only message logic.
