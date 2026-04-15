# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- widened ordered step 2 coverage to the nearby global function-pointer-value family by proving `int (*gp)(int) = inc; int (*p)(int) = gp; return p(4);` stays on the honest loaded-pointer-to-indirect-call route through semantic BIR instead of collapsing to a direct symbol call
- added `backend_codegen_route_x86_64_global_function_pointer_value_call_observe_semantic_bir` and tightened local pointer-slot provenance tracking so pointer values loaded from other pointer objects still emit `bir.load_local ptr %lv.p` plus an indirect `bir.call`, while direct symbol-address slots keep their existing direct-alias behavior

## Suggested Next
- stay on ordered step 2 only if there is another nearby loaded-global-pointer semantic-call family that widens shared provenance handling without drifting into local member-array decay, unrelated pointer-constant repair, or prepare-only legality work
- otherwise pivot intentionally to ordered step 3 and pick one remaining runtime/intrinsic family whose semantic BIR path still lacks first-class proof or honest unsupported bucketing
- leave step 4 note work parked unless a later call/runtime packet exposes a genuinely unbucketed unsupported path that the current semantic-family boundary no longer describes honestly

## Watchouts
- the bug was a provenance shortcut inside semantic lowering: a local slot fed from a loaded pointer object could be re-aliased into a direct symbol and erase the indirect-call route
- keep the new guard scoped to loaded-pointer-object provenance only; do not regress the already-accepted direct-symbol or honest direct-global address shortcuts by forcing every function-pointer slot back through an indirect call

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed for this slice; `test_after.log` shows `100% tests passed, 0 tests failed out of 60`, including `backend_codegen_route_x86_64_global_function_pointer_value_call_observe_semantic_bir`
- proof log preserved at `test_after.log`
