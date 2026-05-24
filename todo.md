Status: Active
Source Idea Path: ideas/open/prealloc-call-boundary-ordering-republication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Adapt AArch64 Call Consumption

# Current Packet

## Just Finished

Completed Step 3: `Adapt AArch64 Call Consumption`.

Adapted the AArch64 explicit after-call move path to construct and read
`plan_prepared_call_boundary_effects` through a narrow local adapter. The
neutral effect records now select the after-call explicit move order while the
existing AArch64 lowering still consumes the original `PreparedMoveResolution`
records for result publication, register views, memory operands, scratch
selection, and final instruction record construction.

## Suggested Next

Continue Step 3 by deciding whether the preservation home population and
republication loops should remain target-local for this plan or get a similarly
narrow neutral-effect ordering adapter.

## Watchouts

The before-call adapter intentionally does not use neutral preservation effect
ordering yet, because current AArch64 behavior emits preservation-home
population before explicit moves. Keep that behavior unless a later slice
explicitly proves and migrates preservation ordering. The after-call adapter
filters explicit effects and remaps their indexes because the neutral full
effect plan also includes preservation-population records before after-call
explicit effects.

## Proof

Ran:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: `159/159` backend tests passed, `0` failed.

Proof log: `test_after.log`.
