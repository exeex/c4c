Status: Active
Source Idea Path: ideas/open/153_phase_c_prealloc_private_cache_contraction_audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Inventory Remaining Prealloc Surfaces

# Current Packet

## Just Finished

Step 2 of `plan.md` completed the remaining prealloc surface inventory in
`docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`.

The inventory groups the residual prepared lookup facade, domain lookup headers
cleaned by ideas 137-150, module/plan aggregates, stack-layout helpers, and
direct AArch64 consumer surfaces. Each group names semantic ownership,
construction-only cache role where present, consumer dependencies, public header
exposure, and the Route 1-7 or route-index prerequisite that keeps it public or
allows later privatization.

## Suggested Next

Step 3 packet: classify Phase B route contraction status without
implementation changes. Use the Step 1 route prerequisite map and the Step 2
surface inventory to produce a route-by-route table in
`docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md` that marks each
prepared lookup/cache/API family as public, private-ready, oracle-only, blocked,
or out-of-scope target policy. Include concrete blockers for Route 4
block-entry publication, Route 5 current-block join-source routing, Route 6
call-use consumers, Route 7 branch/comparison policy, the partial route-index
facade, and the return-chain no-route gap.

## Watchouts

- This plan is analysis-only; do not perform implementation changes during
  Phase C.
- `PreparedFunctionLookups` remains the strongest aggregate-cache contraction
  candidate, but AArch64 still projects and reads its fields directly.
- Return-chain lookup helpers remain in `prepared_lookups.hpp` and have no
  Route 1-7 prerequisite; do not silently classify them as Route 1 or Route 7.
- Module/plan/stack-layout records often carry ABI, frame, register, relocation,
  TLS, move-order, or emission policy. Classify only semantic lookup/cache
  layers as BIR contraction candidates.
- The route index facade covers selected Route 4 and Route 7 references only;
  Routes 1, 2, 3, 5, and 6 retain typed record/index shapes.

## Proof

Command: `printf 'analysis-only Step 2; no build/test required\n' > test_after.log`

Result: passed. This was an analysis-only packet; no build or test was required
by the delegated proof. Proof log path: `test_after.log`.
