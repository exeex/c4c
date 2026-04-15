# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 4, tighten the semantic unsupported boundary

# Current Packet

## Just Finished
- preserved the most specific semantic family when semantic BIR lowering fails at module scope, so module notes now carry the latest runtime/call family miss instead of collapsing everything to the generic admitted-capability umbrella
- added direct backend unit coverage for an unsupported aggregate-return inline-asm shape, proving the failure path keeps `inline-asm placeholder family` visible in both the function note and the module note

## Suggested Next
- keep step 4 focused on planner-facing unsupported-boundary cleanup instead of stretching inline asm further without a real multi-output carrier
- audit whether any remaining semantic-call or runtime failures still lose their family identity through later wrapper notes, and add the next direct note-level regression only if it tightens that contract rather than naming another testcase

## Watchouts
- the lowering pipeline still emits wrapper failures such as `scalar/local-memory semantic family` after specific runtime/call misses; step-4 reporting should prefer the more precise family when summarizing module failure state
- this packet tightens unsupported reporting only; it does not expand inline-asm carrier representation, so tied digit and multi-output forms remain representational follow-on work rather than backend fixes

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with the new `backend_lir_to_bir_notes` unit alongside the existing backend subset
- proof log preserved at `test_after.log`
