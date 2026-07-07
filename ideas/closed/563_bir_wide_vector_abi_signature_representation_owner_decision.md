# BIR Wide Vector ABI Signature Representation Owner Decision

Status: Closed
Type: ABI representation owner follow-up
Parent: `ideas/closed/561_bir_function_signature_semantic_producer_admission.md`
Owning Layer: BIR ABI representation / signature carrier contract

## Goal

Decide and implement the correct BIR ABI representation contract for wide LLVM
vector function signatures before admitting them through return-info or
parameter-layout publication.

## Why This Exists

The function-signature producer lane proved and repaired small fixed-vector and
empty-struct signature publication, but deliberately left larger LLVM vector
signatures fail-closed. Examples include `<4 x float>`, `<8 x i32>`, and
`<4 x i64>` signatures, where the current BIR ABI metadata model does not yet
define whether the value should be represented as scalar carriers, multi-lane
vector carriers, split registers, memory, or another explicit ABI form.

Admitting those signatures inside `call_abi.cpp` without that contract would
publish misleading scalar, VRM, or ad hoc multi-register metadata.

## In Scope

- Define the BIR ABI carrier contract for 16-byte and 32-byte LLVM vector
  return and parameter signatures.
- Repair the real return-info and parameter-layout publication path only after
  the carrier contract is explicit.
- Add focused BIR coverage for admitted wide-vector signatures or for the
  chosen fail-closed owner boundary.
- Prove representative rows such as `src/ieee/pr72824-2.c` / `foo` and
  `src/pr70903.c` / `foo`, or stronger current substitutes.

## Out Of Scope

- Scalar-binop, scalar-cast, local-memory, alloca, or RV64 object-emission
  repair exposed after signature admission.
- Expectation rewrites, unsupported downgrades, allowlist changes, or
  classification-only movement.
- Named-case handling for individual torture rows without a general ABI
  representation rule.
- Broad call/ABI rewrites that leave the same wide-vector signature carrier
  ambiguity in place.

## Acceptance Criteria

- The wide-vector ABI representation contract is explicit and test-covered.
- Return-info and parameter-layout publication either admits the supported
  wide-vector carrier shape or rejects it fail-closed at the documented owner
  boundary.
- Representative wide-vector rows move beyond the old ambiguous
  function-signature failure only when the published BIR signature facts are
  correct.
- Any newly exposed failures are recorded as downstream owner boundaries
  rather than absorbed into this idea.

## Reviewer Reject Signals

- Reject mapping `<4 x float>`, `<8 x i32>`, `<4 x i64>`, or similar wide
  vectors to `I128`, scalar integer pairs, VRM placeholders, or ad hoc
  multi-register metadata without an explicit BIR ABI carrier contract.
- Reject named-case shortcuts for `src/ieee/pr72824-2.c`, `src/pr70903.c`, or
  any single vector spelling instead of a semantic representation rule.
- Reject expectation, unsupported-marker, allowlist, or classification changes
  claimed as ABI representation progress.
- Reject broad call/ABI rewrites that retain the same fail-closed wide-vector
  signature ambiguity under a new helper name.
- Reject routing downstream scalar-binop, scalar-cast, local-memory, alloca, or
  object-emission failures back into this owner decision after signature facts
  are correctly published.

## Closure Note

Closed after Step 3 validation. The accepted BIR ABI signature carrier contract
admits fixed vectors over 8 and up to 16 bytes through the explicit `I128`
memory ABI carrier, while vectors over 16 bytes remain fail-closed at the
documented function-signature owner boundary. Focused coverage includes 16-byte
parameter and return publication plus 32-byte return and parameter-only
fail-closed assertions.

Close gate used the canonical backend subset logs: `test_before.log` and
`test_after.log` both reported 346/346 passing, and the regression guard passed
with non-decreasing pass-count mode. The rejected full-suite baseline candidate
failed only `string_authority_guard`, which reports pre-existing string
authority entries outside this wide-vector ABI signature owner slice.
