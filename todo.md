Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Thin `dispatch.cpp` Materialization Touchpoints

# Current Packet

## Just Finished

Step 4 audit packet complete. AST-backed checks on `dispatch.cpp` confirmed the
remaining materialization/publication-shaped dispatch calls are routing
handoffs, not leaf materialization implementations, after the Step 4 helper
moves. `rg` scans confirmed `dispatch.cpp` still includes
`dispatch_value_materialization.hpp` only to install the central
`emit_value_publication_to_register` fan-out bridge into branch-fusion hooks;
there is no direct `emit_value_publication_to_register(...)` call in
`dispatch.cpp`.

Remaining touchpoint classification:

- Thin explicit delegation to narrow owners:
  `lower_address_materialization`, `lower_address_materializations`,
  `make_block_address_materialization_index`, and
  `make_load_global_got_materialization_instruction` delegate to the
  globals/address-materialization owner; store/pointer publication routes
  delegate to `memory_store_sources.*`; scalar, FP, and cast routes delegate to
  `alu.*`, `fp_value_materialization.*`, and `cast_ops.*`; local-slot, fixed
  formal, compare-condition, memory-result record/retarget, and fused-compare
  publication routes delegate to `dispatch_publication.*`; call-boundary
  materialization routes delegate to `calls_dispatch_bridge.*`.
- Intentional central fan-out bridge: `#include
  "dispatch_value_materialization.hpp"` plus the
  `make_dispatch_branch_fusion_hooks` function-pointer assignment for
  `emit_value_publication_to_register`. This bridge still recursively consumes
  most leaf value materializers and should stay central unless a separate
  fan-out split is delegated.
- No small hidden materialization decision with a clear existing owner was
  found. The two local helpers named
  `lower_store_local_with_address_materialization` and
  `lower_scalar_with_address_materialization` are dispatch-ordering bridges:
  they sequence address materialization before memory/scalar lowering, but the
  actual materialization and publication decisions are delegated to the narrow
  owners above.

## Suggested Next

Step 4 can advance to Step 5 in the next lifecycle update. Next coherent
packet: begin Step 5 coverage work by auditing the existing focused
materialization tests against the moved helper families and adding or
tightening only the smallest missing coverage.

## Watchouts

- Keep this route focused on value materialization authority.
- Do not fold in edge-copy cleanup, publication cleanup, calls cleanup, or
  broad AArch64 pipeline redesign.
- Treat expectation weakening, unsupported downgrades, text-emission contracts,
  and named-test shortcuts as route failures.
- Leave unrelated transient `review/` artifacts untouched.
- `emit_value_publication_to_register` is the fan-out point and should not be
  the first move; it recursively consumes most leaf materializers and is used by
  calls, stores, edge copies, publication, and branch fusion hooks.
- The generic dispatcher now calls
  `emit_prepared_value_home_publication_to_register` as the narrow bridge into
  the new owner. Keep the two plan helpers private unless a later packet has a
  concrete direct client.
- The branch-fusion hook struct still carries a function pointer named
  `emit_prepared_value_home_to_register`; the function declaration itself now
  comes from `prepared_value_home_materialization.hpp`.
- `fp_value_materialization.cpp` still depends on generic
  `emit_value_publication_to_register` for integer compare/select and cast
  materialization subpaths; keep that bridge intentional unless a later packet
  splits those dependencies semantically.
- The local dispatch ordering bridges around address materialization are
  intentionally order-sensitive. Do not move them casually into globals, memory,
  or ALU without a separate packet that owns cross-owner ordering and proof.
- Step 5 should prefer semantic machine-instruction records and prepared facts
  over text-emission expectations.

## Proof

Audit-only proof requested; no code tests were run and `test_after.log` was not
updated by this packet.

Command run exactly:
`git diff --check -- todo.md`

Result: passed.
