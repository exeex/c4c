Status: Active
Source Idea Path: ideas/open/call-boundary-move-classification-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Reuse Path For x86 Prepared Calls

# Current Packet

## Just Finished

Step 4 added concrete x86 prepared reuse evidence for the shared prealloc
call-boundary classification helper.

`x86::prepared::Query` now exposes prepared call plans through `call_plans()`
and provides `classify_call_boundary_move(...)`, a narrow forwarding helper
that returns `prepare::PreparedCallBoundaryMoveClassification` from
`prepare::classify_prepared_call_boundary_move(...)`.

Focused x86 prepared query coverage now builds call-boundary Prepared facts,
classifies a call-argument move through the x86 query surface, verifies phase,
destination role, storage kind, ABI index, matched argument plan, matched ABI
binding, and source Prepared record pointers, and checks missing-binding status
without falling back to target-local raw decoding.

## Suggested Next

Step 5 should validate the completed route and summarize the anti-overfit
coverage: prealloc helper coverage, AArch64 consumer coverage, and x86 prepared
reuse coverage, with no test weakening or expectation reclassification.

## Watchouts

x86 now proves the shared classification can be reused from a target prepared
query surface. This remains evidence only; x86 lowering, x86 operand spelling,
ABI policy, and emission were not rewritten.

The shared classification still returns facts/status only. Keep preservation
lookup/emission and target-specific operand construction out of prealloc unless
a later packet explicitly owns that extraction.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 153/153 backend tests. Proof log: `test_after.log`.
