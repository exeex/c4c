Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidate Proof And Residue Audit

# Current Packet

## Just Finished

Completed Step 5 by re-scanning the migrated AArch64 call/publication helpers
and the idea 69 history from activation through `HEAD`.

The migrated call helper surface in `calls.cpp` now gates register-argument
publication through `PreparedCallBoundaryEffectPlan`, keeps
`plan_prepared_call_boundary_effects` as the call-boundary authority, and
leaves AArch64 register rendering, instruction choice, and machine-record
construction local. No broad call rewrite or renamed local re-derivation was
found behind the prepared-effect helper names.

The migrated publication helper surface in `dispatch_publication.cpp` routes
same-block and fused-compare source decisions through prepared
edge-publication source producer queries, and fixed-formal store-local
publication still consumes `plan_prepared_fixed_formal_store_source_publication`
before target-local `str*` emission. No missed store-source migration was found
inside `dispatch_publication.cpp`.

Step 4 residue remains limited to the typed stack-source edge-copy owner
captured in
`ideas/open/78_aarch64_edge_copy_typed_stack_source_prepared_authority.md`.
That residue is not a missed `dispatch_publication.cpp` packet: this file still
has no valid `PreparedEdgePublication` consumer to pass to
`prepare_same_width_i32_stack_source_publication` without expanding the active
idea into `dispatch_edge_copies.cpp`.

Audit result: no weakened expectations, unsupported downgrades, broad dispatch
rewrite, or testcase-shaped/publication-helper shortcut was found in the idea
69 diff. The code diff is limited to `calls.cpp` and
`dispatch_publication.cpp`; no tests were changed.

## Suggested Next

Ask the plan owner to evaluate whether the active idea 69 runbook should close
or be retired with follow-up idea 78 left open for the edge-copy typed
stack-source owner.

## Watchouts

- Keep idea 78 separate; do not thread `PreparedEdgePublication` into
  `dispatch_publication.cpp` solely to create a typed stack-source consumer.
- `lower_fixed_formal_store_local_publication` should remain target-local for
  `strb`/`strh`/`str` selection, register view resizing, and AArch64 address
  rendering.
- The only pre-existing dirty tree item observed during this packet was
  `review/idea69_step2_prepared_effect_review.md`; it was left untouched.

## Proof

Command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed; build succeeded and 169/169 matching backend tests passed.
Proof log: `test_after.log`.
