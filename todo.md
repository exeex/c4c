Status: Active
Source Idea Path: ideas/open/558_bir_call_metadata_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Direct-Call Producer Boundary

# Current Packet

## Just Finished

Lifecycle activation selected Step 1 - Inspect Direct-Call Producer Boundary
from `ideas/open/558_bir_call_metadata_semantic_producer_admission.md`.

## Suggested Next

Inspect the `src/20000412-2.c` direct-call representative, current per-case
log evidence, `src/backend/bir/lir_to_bir/calling.cpp`, adjacent semantic call
admission code, and focused BIR call tests. Record the exact missing call
metadata boundary before implementation.

## Watchouts

Reject downstream RV64/MIR call inference, generic local-memory routing,
runtime/intrinsic repairs, expectation rewrites, unsupported-marker changes,
allowlist edits, runtime-comparison changes, and named-case shortcuts. The
runbook must cover call-return metadata before claiming the source idea is
complete.

## Proof

Lifecycle-only activation. No build proof required by the plan owner.
