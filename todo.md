Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Consumer Use And Decide Closure
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3.3 is now complete: grouped spill/reload publication has direct vector,
grouped GPR, and width-2 float proof, including call-crossing eviction from
the lone legal grouped callee span, truthful `spill_register_authority`,
matching grouped spill/reload ops, and preserved grouped stack-storage
identity across liveness, direct prepared consumption, and prepared-printer
surfaces.

## Suggested Next

Execute Step 4 by treating the grouped-width substeps as complete, using the
fresh backend build and `^backend_` proof as the current validation baseline,
and deciding whether any remaining consumer-use gaps still belong to idea 91
or should be split into follow-on work before closure is considered.

## Watchouts

- Do not close idea 91 directly from Step 3.3 completion alone; Step 4 still
  owns the final consumer-use review and leftover classification.
- The supervisor's before/after `^backend_` runs were green, but the
  monotonic regression-guard script did not produce a closure-ready pass
  because the strengthened proof lives inside existing backend test binaries
  rather than increasing the CTest pass count.
- Keep idea 90 out-of-SSA follow-on work and any width-aware pool-policy
  redesign outside this Step 4 packet unless the remaining gap is proven to be
  required for idea 91 closure.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: baseline commit `14c7bdde` and current `HEAD` both passed; this is the
current validation baseline for Step 4 closure judgment.
Log: `test_after.log`
