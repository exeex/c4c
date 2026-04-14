# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md

## Activation Decision

- 2026-04-14 activation result:
  activate `ideas/open/47_semantic_call_runtime_boundary.md` as the next
  executable backend follow-on
- rationale:
  this is the immediate continuation of the shared semantic-lowering route
  after the closed backend-reboot idea; `ideas/open/48_prepare_pipeline_rebuild.md`
  and `ideas/open/49_prepared_bir_target_ingestion.md` both depend on semantic
  BIR staying honest first, and idea 49 explicitly depends on both earlier
  tracks
- route constraint:
  keep this runbook focused on semantic call/runtime boundary work and do not
  silently pull `prepare` reconstruction or prepared-BIR target-ingestion work
  forward

## Current Active Item

- active route:
  semantic BIR remains the truth surface, `prepare` remains the legality
  owner, and target reconnection stays deferred to later follow-on ideas
- current capability family:
  semantic call ownership and runtime-facing lowering
- current packet focus:
  semantic runtime/intrinsic lowering now starts with integer `abs` in
  `src/backend/lowering/lir_to_bir_calling.cpp`; the next packet should stay
  on adjacent runtime/call families instead of drifting into `prepare`
- packet rule:
  the first executor packet must change shared semantic lowering, not just add
  new test observers or rename unsupported cases
- proof expectation:
  choose a proving subset from backend-route or internal backend cases that
  exercises one semantic call/runtime family with a fresh build before broader
  validation
- Just Finished:
  lowered `LirAbsOp` into semantic BIR as `bir.sub` plus `bir.select` for
  supported integer widths, and added dual backend-route proof for `abs(int)`
  and `labs(long)`
- Watchouts:
  runtime/intrinsic follow-ons still missing from semantic BIR include the
  dedicated LIR intrinsic families (`memcpy`, `va_*`, `stacksave`,
  `stackrestore`); keep them owned by shared semantic lowering instead of
  falling back to target-shaped handling
- Proof:
  `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
  passed; proof log at `test_after.log`

## Suggested Next

- extend semantic runtime lowering with one next intrinsic family that already
  has shared LIR shape and nearby coverage, with `memcpy` the most natural
  follow-on if it can be represented without introducing target-specific
  contracts
- if the next runtime family needs new semantic BIR surface, keep that surface
  shared and planner-honest rather than encoding direct target behavior
