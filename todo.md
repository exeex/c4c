Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended `plan.md` Step 3 by preserving raw pointer-value provenance when a
pointer SSA is spilled through a local pointer slot and reloaded. That shared
local-memory repair moves `00182` out of the store local-memory bucket: `topline`
now reaches the next honest blocker in scalar control flow, while the earlier
`00078` / `00170` / `00171` prepared-module handoff advances remain intact.
The named proving cluster is still not coherent, because `00163` remains in
store local-memory and appears to straddle the separate global/data family.

## Suggested Next

Have the supervisor decide whether the active Step 2 proving cluster needs a
route repair before more Step 3 work lands. `00182` is no longer a pure
local-memory probe now that it reaches `topline` scalar-control-flow, `00078`
/ `00170` / `00171` already stop at prepared-module/control-flow boundaries,
and `00163` still mixes local pointer slots with aggregate-backed global/data
traffic. The next honest packet is likely a re-baseline or lifecycle decision,
not another blind store-local-memory patch.

## Watchouts

- Keep the admitted `00131` / `00211` / `00210` prepared-module lane as
  regression baseline coverage.
- Keep `00189` explicit as the adjacent indirect/global-function-pointer plus
  indirect variadic-runtime boundary rather than silently widening into it.
- `00078` now reaches the prepared-module handoff and fails there as a
  multi-defined same-module-symbol-call lane with extra local-memory/control-
  flow pressure, so it is no longer a pure semantic-local-memory probe.
- `00171` now reaches the prepared-module handoff and fails there as a
  compare-against-zero plus runtime-call/control-flow shape, so it is also no
  longer a pure semantic-local-memory probe.
- `00170` now reaches the prepared-module handoff as a multi-function same-
  module-symbol lane after the pointer-param load repair, so it also left the
  pure semantic local-memory bucket.
- `00182` no longer fails in store local-memory; `topline` now fails in
  scalar-control-flow because its switch fanout is the next honest blocker
  after the pointer-slot provenance repair.
- `00163` mixes local pointer slots with aggregate-backed global address and
  field traffic, so its remaining store-local-memory failure may be straddling
  the source idea's separate global/data family instead of a pure `&local`
  lane.
- Keep `00207` out of the next packet; its VLA/alloca plus goto shape is not
  the same bounded lane as direct addressed local scalars.
- Keep `00176` and `00217` out of the next packet; they widen into indexed
  `gep` traffic rather than direct addressed local scalar dereferences.
- Keep `00057` and `00124` out of the next packet; they remain emitter and
  scalar-control-flow neighbors rather than the named local-memory cluster.
- Do not weaken `x86_backend` expectations or add testcase-shaped recognizers.

## Proof

Step 3 proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00078_c|c_testsuite_x86_backend_src_00163_c|c_testsuite_x86_backend_src_00170_c|c_testsuite_x86_backend_src_00171_c|c_testsuite_x86_backend_src_00182_c|c_testsuite_x86_backend_src_00131_c|c_testsuite_x86_backend_src_00211_c|c_testsuite_x86_backend_src_00210_c|c_testsuite_x86_backend_src_00189_c)$' > test_after.log 2>&1`

Result: `backend_lir_to_bir_notes`, `backend_x86_handoff_boundary`,
`c_testsuite_x86_backend_src_00131_c`, `c_testsuite_x86_backend_src_00211_c`,
and `c_testsuite_x86_backend_src_00210_c` pass; `00189` remains the explicit
prepared-module boundary failure. `00078`, `00170`, and `00171` stay at
prepared-module/control-flow boundaries rather than regressing into semantic
local-memory. `00182` now fails in scalar-control-flow instead of store
local-memory, while `00163` still fails in store local-memory. The packet
shows real shared semantic progress but leaves the proving-cluster route
blocked and needing supervisor review. Proof log path: `test_after.log`.
