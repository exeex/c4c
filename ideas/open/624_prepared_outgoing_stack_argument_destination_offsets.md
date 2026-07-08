# Prepared Outgoing Stack Argument Destination Offsets

Status: Open
Type: Implementation
Parent: `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Related:
- `review/613_step2_byval_outgoing_stack_slice_review.md`
- `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Owning Layer: prepared call-boundary authority production
Queue Order: 23
Prerequisites: ordinary call argument preparation must already identify byval or stack-copy aggregate argument payloads and their outgoing stack area
Estimated Evidence Breadth: current blockers include `src/20000808-1.c` plus broader aggregate/outgoing-stack argument transport rows; refresh diagnostics before implementation
Proof Surface: prepared call argument move/binding facts, outgoing stack-area destination offsets, and RV64 call ABI consumer fail-closed guards

## Goal

Publish explicit prepared destination stack offsets and sizes for outgoing
stack-slot call arguments, including byval aggregate stack-copy payloads, so
RV64 call consumers can lower them without inferring ABI layout locally.

## Why This Exists

Idea 613 is an ABI/RV64 consumer route. A reviewed Step 2 attempt added a
consumer for complete byval outgoing-stack facts, but the representative real
row `src/20000808-1.c` still lacked prepared destination stack offsets on the
call-argument move or binding facts. That missing authority belongs to the
prepared call-boundary producer layer, not to the RV64 consumer.

Step 5 close-readiness classification for idea 613 also assigned adjacent
aggregate/outgoing-stack argument transport rows such as the `931004-*`
family, `src/931031-1.c`, `src/950607-2.c`, and `src/pr69447.c` to this
authority route. Those examples are breadth evidence for outgoing stack
destination publication, not named-case implementation targets.

## In Scope

- Produce explicit destination stack offset and size facts for prepared
  outgoing stack-slot call arguments.
- Cover byval aggregate stack-copy arguments whose payload is transported to
  the outgoing stack area.
- Preserve enough source and transport identity for downstream RV64 consumers
  to verify matching argument, destination, size, and payload coverage.
- Keep diagnostics fail-closed when the outgoing stack area, destination
  offset, destination size, or payload coverage is incomplete.

## Out Of Scope

- RV64 object-emission lowering of the completed facts.
- Variadic or library call policy.
- Runtime mismatch triage.
- Local/global producer repairs, stack-frame consumer repairs, expectation
  changes, unsupported marker changes, allowlists, timeouts, or accounting.
- Inferring destination offsets inside final assembly emission from testcase
  names, ABI indices alone, or final object shape.

## Acceptance Criteria

- At least one ordinary same-module byval or outgoing-stack aggregate row
  exposes explicit prepared destination stack offsets and sizes before RV64
  object emission.
- `src/20000808-1.c` either moves past the missing-prepared-authority blocker
  or is reclassified with a more precise non-producer blocker backed by current
  diagnostics.
- RV64 consumers can require the explicit facts instead of deriving stack
  offsets from ABI index or final assembly shape.
- Negative proof keeps incomplete outgoing stack area, missing destination
  offset, undersized destination size, and conflicting destination facts
  rejected.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts that special-case `src/20000808-1.c` or
  any named torture row.
- Reject RV64 consumer code that infers outgoing destination offsets from ABI
  indices, final assembly layout, or testcase names instead of consuming
  prepared facts.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or classification-only changes that leave the prepared
  call-argument facts without explicit destination stack offsets and sizes.
- Reject broad ABI, variadic, library, local/global producer, or runtime
  rewrites that are not needed to publish the outgoing stack destination
  authority.
