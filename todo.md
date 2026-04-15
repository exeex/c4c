# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: route checkpoint across ordered steps 2 and 3 before more execution

# Current Packet

## Just Finished
- recorded a reviewer checkpoint after 65 commits since activation because execution had outpaced the current transcript
- confirmed that `plan.md` still matches `ideas/open/47_semantic_call_runtime_boundary.md`, but `todo.md` had fallen behind the landed semantic-BIR surface
- recognized that the active backend route now spans step-2 call widening plus step-3 runtime-family lowering, including variadic/indirect calls, `va_*`, `stacksave`/`stackrestore`, inline asm, `abs`/`fabs`, `memcpy`, and `memset`

## Suggested Next
- take a broader backend validation checkpoint from a fresh build before authorizing another executor packet
- if that broader checkpoint fails, route the next packet to the first uncovered semantic family revealed by the failures instead of adding more proof-only observer cases
- if the broader checkpoint passes, pick the next bounded packet from an uncovered semantic family that is still missing semantic lowering or unsupported-boundary cleanup rather than extending the existing variadic proof cluster

## Watchouts
- do not add another proof-only `tests/backend/CMakeLists.txt` slice before the broader backend checkpoint lands
- keep new work under `BirFunctionLowerer` and the split `lir_to_bir_*` owners; do not revive `call_decode.cpp` or push ABI legality back into semantic lowering
- prefer semantic-family gaps over testcase-neighbor padding when selecting the next executor packet

## Proof
- the reviewer checkpoint judged the route `drifting`, the source idea still matched, and the immediate repair as `todo.md` only
- narrow variadic proof remains the last accepted packet evidence; no fresh broader `test_after.log` exists yet for the full backend checkpoint
- next supervisor proof action should be a fresh build plus broader backend validation, then canonical log roll-forward on success
