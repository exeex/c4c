# Prepared Return Destination Home Authority

Status: Open
Type: Implementation
Parent: `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Related:
- `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Owning Layer: prepared return authority production
Queue Order: 29
Prerequisites: return preparation must identify return value homes, source values, destination homes, and required stack-to-register or register-to-home transfers
Estimated Evidence Breadth: return move-bundle rows represented by `src/20001130-2.c` and `src/20080719-1.c`; refresh diagnostics before implementation
Proof Surface: prepared return facts, destination-home publication, return move-bundle target guards

## Goal

Publish explicit prepared destination-home authority for return values so RV64
return move-bundle lowering can consume concrete homes without inferring
return transfers from ABI convention, final assembly shape, or source-file
behavior.

## Why This Exists

Idea 613 included return move-bundle target handling only when prepared return
facts were complete. Step 3 and Step 5 classification found return rows whose
prepared facts still lack destination-home authority, including
`return_stack_to_register` shapes. That missing publication belongs to return
authority production before RV64 object emission can lower the transfer
semantically.

## In Scope

- Produce explicit prepared return destination homes for scalar return values
  that require stack-to-register, register-to-home, or equivalent return
  transfers.
- Preserve return source identity, ABI return register or stack home, width,
  and function-return association for downstream consumers.
- Keep diagnostics fail-closed when return source, destination home, width, or
  transfer authority is missing, ambiguous, or only derivable from convention.
- Reclassify representative return rows if refreshed diagnostics prove a more
  precise non-return-authority owner.

## Out Of Scope

- Ordinary scalar call argument/result transport already handled by idea 613.
- Pointer stack-result policy covered by idea 627.
- FPR ABI/frame policy covered by idea 628.
- Generic move-bundle authority unrelated to function returns.
- Runtime mismatch triage, local/global producer repair, variadic/library
  policy, expectation changes, unsupported marker changes, allowlists,
  timeouts, or accounting.

## Acceptance Criteria

- At least one return move-bundle row exposes explicit prepared destination
  home authority before RV64 object emission.
- `return_stack_to_register` or adjacent return-transfer rows move past the
  missing destination-home blocker or are reclassified to a precise owner with
  current diagnostics.
- RV64 return consumers require explicit return destination-home facts and
  remain fail-closed when those facts are absent.
- Negative proof keeps non-return move-bundles, pointer stack-results, FPR
  policy, runtime, variadic/library, local/global, and generic producer gaps
  outside this idea.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts for `src/20001130-2.c`,
  `src/20080719-1.c`, or adjacent return rows.
- Reject RV64 object-emission inference of return destination homes from ABI
  convention alone, final assembly layout, source filenames, or runtime
  behavior.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject broad move-bundle rewrites that do not specifically publish prepared
  function-return destination homes.
- Reject helper renames or diagnostic wording changes that leave return
  destination-home authority missing.
