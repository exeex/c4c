# RV64 Residual Pointer Array Select Flow

## Goal

Classify and repair the remaining RV64 pointer/local-array/select runtime
residuals that were deliberately split out of idea 312 closure because they are
not one local frame-slot address materialization rule.

## Why This Exists

The idea 312 Step 5 sweep left four runtime failures that are adjacent to local
address work but no longer match the completed local frame-slot publication,
local array pointer-step, or byte element access repairs. They need a separate
triage route so fixes do not overfit one candidate or silently expand aggregate
and function-pointer work.

## In Scope

- Pointer-to-pointer local chains after the first dereference.
- Array parameter to local array pointer flow.
- Indexed local array select/update chains when local array addressing is only
  one part of the failure.
- Pointer `select`, `inttoptr`, and `ptrtoint` conditional flow when it is the
  first bad runtime mechanism.
- Reclassification into narrower ideas if these candidates split further.

## Out Of Scope

- Core local frame-slot address publication already closed by idea 312.
- Aggregate-local subobject/byval repair tracked separately.
- Function-pointer address, return, or indirect-call repair tracked separately.
- Broad switch/control-flow repair unless it is required to prove a selected
  pointer/array value flow.

## Candidate Evidence

- `src/00005.c`: pointer-to-pointer local residual after the first dereference.
- `src/00077.c`: array parameter/local array pointer flow.
- `src/00143.c`: broad indexed local array select/update chains plus
  switch-shaped control flow.
- `src/00144.c`: pointer select, `inttoptr`, and `ptrtoint` conditional flow.

## Acceptance Criteria

- The route first separates these candidates into coherent first-bad
  mechanisms using prepared BIR, emitted assembly, and qemu evidence.
- Any implementation packet repairs a semantic pointer/array/select lowering
  rule and proves nearby same-feature cases.
- Remaining broad control-flow or ABI residue is reclassified into its own
  durable idea instead of absorbed into this route.

## Reviewer Reject Signals

- A patch targets only one candidate filename or one observed stack offset
  without proving a semantic pointer/array/select lowering rule.
- `00143` is claimed fixed by skipping switch-shaped flow or by weakening the
  runtime contract.
- `00144` is treated as ordinary local address materialization while
  pointer-integer conversion or select value flow remains unhandled.
- The route mixes aggregate byval or function-pointer indirect-call work into
  this idea without a lifecycle split.
- Evidence-only classification is claimed as capability progress without a
  matching backend or runtime proof.
