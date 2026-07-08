# Pointer Stack-Result Call Policy

Status: Open
Type: Implementation
Parent: `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Related:
- `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Owning Layer: ABI policy and prepared call-result authority
Queue Order: 27
Prerequisites: ordinary same-module call/result preparation must distinguish pointer-valued results, result homes, and stack-slot or aggregate destinations
Estimated Evidence Breadth: pointer call-result rows represented by `src/20030715-1.c`, `src/20011113-1.c`, `src/20041218-1.c`, `src/pr20601-1.c`, `src/pr34176.c`, and `src/pr58209.c`
Proof Surface: prepared call-result facts, pointer result destination policy, RV64 consumer fail-closed guards

## Goal

Define and publish semantic authority for pointer-valued call results whose
destination is a stack slot or aggregate home, so downstream RV64 lowering can
consume explicit result-home facts instead of deriving stack-result behavior
from source filenames, final assembly shape, or ad hoc pointer treatment.

## Why This Exists

Idea 613 repaired ordinary scalar GPR call/result consumers when complete
prepared facts already existed. Its final residual classification found a
separate pointer stack-result family still labeled as ABI failure but blocked
on policy/authority: RV64 object emission cannot decide whether and how to
materialize pointer call results into stack homes without explicit prepared
result destination facts.

## In Scope

- Classify pointer-valued ordinary same-module call results by prepared result
  destination and stack-home authority.
- Publish or consume explicit facts for pointer result destinations when the
  upstream call-result route already has enough semantic authority.
- Preserve value identity, destination stack slot or aggregate home, width, and
  callsite association needed by RV64 consumers.
- Keep diagnostics fail-closed when pointer stack-result policy, destination
  home, width, or source/result identity is missing or ambiguous.

## Out Of Scope

- Generic scalar GPR argument transport already handled by idea 613.
- Outgoing stack aggregate argument destination offsets covered by idea 624.
- Stack-slot preserve source publication covered by idea 625.
- Dynamic-frame callee-saved placement covered by idea 626.
- FPR ABI/frame policy, variadic or library call policy, runtime mismatch
  triage, local/global producer repair, expectation changes, unsupported
  marker changes, allowlists, timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe identifies a pointer stack-result family with shared
  authority rather than a named-case-only row.
- At least one pointer-valued ordinary call-result row moves past the missing
  pointer stack-result authority blocker or is reclassified to a more precise
  non-policy owner backed by diagnostics.
- RV64 consumers require explicit pointer result destination/home facts and
  remain fail-closed when those facts are absent.
- Negative proof keeps scalar-only, aggregate outgoing-stack, FPR, variadic,
  library, runtime, local/global, and missing-authority rows outside this idea.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts for any representative source file named in
  this idea.
- Reject RV64 object-emission inference of pointer stack-result homes from
  final assembly layout, ABI register names alone, source filenames, or
  aggregate shape.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject broad ABI rewrites that fold aggregate outgoing-stack arguments, FPR
  policy, dynamic-frame placement, local/global producers, or runtime policy
  into this pointer stack-result route.
- Reject helper renames or diagnostic wording changes that leave pointer
  result destination-home authority unpublished or unconsumed.
