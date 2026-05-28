Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Bind Each Focused Probe To One Backend Owner

# Current Packet

## Just Finished

Completed Step 4 - Bind Each Focused Probe To One Backend Owner for the active
decomposition route in
`ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md`.

Selected next implementation seam: GOT-required `LoadGlobal` materialization.

Selected focused probe:

- Add `tests/backend/case/aarch64_got_load_global_prepared_memory.c` and
  register it as
  `backend_codegen_route_aarch64_got_load_global_prepared_memory`.
- The focused route probe should require GOT page/low12 materialization for a
  GOT-required external global, including `:got:external_data_symbol`,
  `:got_lo12:external_data_symbol`, and the final load through the materialized
  GOT entry.
- The focused route probe should forbid direct non-GOT global addressing for
  the same symbol, especially direct `:lo12:external_data_symbol`.
- Existing `backend_aarch64_instruction_dispatch` subtests
  `load_global_call_argument_uses_got_for_got_required_global`,
  `stack_preserved_loaded_global_pointer_publishes_before_call_argument_reload`,
  and `load_global_call_argument_keeps_direct_for_direct_global` remain
  integration/focused-extraction candidates, not the only proof.

Backend owner binding:

- Primary owner surface:
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`,
  specifically the prepared global-load materialization path around
  `emit_prepared_global_load_to_register`.
- Helper boundary:
  AArch64 global-address relocation helpers in `globals.cpp`/`globals.hpp`
  may supply the GOT-vs-direct address materialization primitive, but the
  selected Step 5 packet should only use that boundary to express the generic
  relocation decision needed by prepared `LoadGlobal`.
- Consumer boundaries:
  dispatch publication, call-argument lowering, and ordinary dispatch use sites
  may consume a register produced for a prepared `LoadGlobal`; they should not
  duplicate GOT-required symbol classification, introduce direct page/low12
  fallback for GOT-required globals, or make call/store/edge-copy policy
  decisions as part of this repair.

Narrow implementation proof command for Step 5:

```sh
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_got_load_global_prepared_memory|backend_aarch64_instruction_dispatch)$'
```

Acceptance focus for that command:

- The new focused route test proves the GOT-required `LoadGlobal` seam without
  relying on the monolithic dispatch test as the only evidence.
- `backend_aarch64_instruction_dispatch` is kept in the same narrow command as
  integration confirmation that the current first bad assertion shrank or moved.
- `c_testsuite_aarch64_backend_src_00196_c` remains outside the Step 5 proof
  because current plan/source-idea evidence keeps it in idea 58's later
  `78730af2f` family unless a focused probe proves shared ownership.

## Suggested Next

Execute Step 5 - Resume Implementation On The Narrowest Generic Seam.

Suggested Step 5 packet:

- implement only the GOT-required prepared `LoadGlobal` materialization rule
  bound above
- create/register only the selected focused probe if it does not already exist
- run the narrow proof command recorded above and write output to
  `test_after.log`

## Watchouts

- Explicit Step 5 do-not-touch boundaries: do not edit call/outgoing argument
  lowering, edge-copy publication, store-global publication, store-local
  publication, fused-compare materialization/order, unrelated backend route
  cases, c-testsuite expectations, `plan.md`, or `ideas/open/*`.
- The only implementation owner surface selected by this packet is prepared
  GOT-required `LoadGlobal` materialization through
  `dispatch_value_materialization.cpp`, with tightly scoped use of existing
  `globals.cpp`/`globals.hpp` relocation helpers if needed.
- Do not satisfy the new focused test by matching testcase names, exact
  temporary names, exact labels, or fixed stack offsets. The semantic rule is
  GOT-required global address materialization for prepared `LoadGlobal`.
- The dirty stack is intentionally preserved. Do not revert it as part of
  decomposition documentation, but do not classify it as accepted progress yet.
- `review/step3_dispatch_route_review.md` described the dirty route as non-
  overfit but drifting; Step 5 should stay on the bound owner/probe instead of
  chasing a new monolithic dispatch assertion.
- New backend route cases under `tests/backend/case/` need matching CMake
  registrations before they are runnable.

## Proof

No fresh build or test proof required or run for this documentation-only Step 4
binding packet.

Evidence used:

- Existing `test_after.log`, showing the current four-test focused proof at
  `2/4` passing and
  `expected GOT-required global load to use GOT page/low12 materialization` as
  the current `backend_aarch64_instruction_dispatch` first bad assertion.
- Active `plan.md`, confirming Step 4 requires the selected focused probe,
  owner surface, proof command, dependencies, and narrowest next implementation
  packet.
- Source idea
  `ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md`,
  confirming GOT-required `LoadGlobal` materialization is a separate seam and
  that implementation must not resume until a focused probe and owner are
  selected.
- Existing Step 3 todo evidence, confirming the selected/proposed focused probe
  and the monolithic C++ route subtests that currently isolate this contract.

Proof log path: `test_after.log`.
