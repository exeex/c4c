Status: Active
Source Idea Path: ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Stage 2 Acceptance Check

# Current Packet

## Just Finished

Step 6: Final Stage 2 Acceptance Check verified the Stage 2 review/layout and
handoff artifacts for the AArch64 module phoenix route. The check confirmed
that `stage2_review_layout.md` exists and covers current subsystem shape,
extraction quality/no-repair status, replacement layout, parent 224 judgment,
and the Stage 3 draft artifact map. It also confirmed that
`stage2_to_stage3_handoff.md` exists, matches the mandatory draft map and route
constraints, `module.cpp` still exists, and `plan.md`/`todo.md` still point to
idea 226.

## Suggested Next

Stage 2 is ready for supervisor review and possible plan-owner closure or
transition decision.

## Watchouts

- Preserve the Stage 1 correction: `src/backend/mir/aarch64/module/module.cpp`
  remains compiled legacy evidence until replacement implementation plus build
  wiring exist.
- Stage 2 does not own implementation edits, build rewiring, test expectation
  changes, or replacement draft contents.
- Step 2 classifies the returned-`add`/`sub` path as an optional fast path with
  overfit pressure; do not expand it case by case during replacement layout.
- Treat `FunctionRecord::machine_nodes`, legacy register-name parsing,
  spelling-based identity recovery, raw source/prepared pointers, and snapshot
  booleans as compatibility evidence, not as primary replacement contracts.
- Step 5 wrote the Stage 3 intake contract only; replacement draft contents
  still belong to Stage 3, not Stage 2.
- The mandatory header set is intentionally limited to `module.hpp.md` by the
  phoenix header policy. `helper.hpp` is only an allowed exception if Stage 3
  explicitly proves a private helper header is needed.
- Keep the parent-224 constraints from Step 3 visible when naming draft files:
  canonical MIR nodes are the carrier, flat vectors are compatibility only,
  cached display strings are diagnostics only, and the shared printer owns
  traversal while AArch64 owns rendering.
- The final acceptance check found no implementation, build wiring, test, or
  expectation dirtiness. This packet only updates `todo.md` and
  `test_after.log`.

## Proof

No build needed for this markdown-only acceptance packet. The delegated proof
wrote concise read-only acceptance notes to `test_after.log`, verifying the
Stage 2 review/layout artifact, the Stage 2 to Stage 3 handoff artifact, the
mandatory Stage 3 draft map and route constraints, the continued existence of
`src/backend/mir/aarch64/module/module.cpp`, clean non-doc implementation/build
/test/expectation state, and continued plan/todo linkage to
`ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md`.
