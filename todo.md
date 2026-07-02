Status: Active
Source Idea Path: ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close-Readiness Review

# Current Packet

## Just Finished

Step 4: close-readiness reviewed the RV64 scalar helper cleanup against
`ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md`.

Decision: ready for plan-owner closure evaluation.

Review findings:

- The active range from plan activation through `HEAD` touches only
  `object_emission.cpp`, `prepared_scalar_emit.cpp`,
  `prepared_scalar_emit.hpp`, and `todo.md`; no tests, expectations,
  unsupported markers, or diagnostics were edited.
- Scalar ownership is narrower: scalar binary, cast, compare branch/value,
  move-to-register/location, simple return, and their narrow support helpers
  now live behind explicit `prepared_scalar_emit.*` declarations instead of
  remaining in the object route.
- `prepared_scalar_emit.*` is not a catch-all monolith for the object route:
  `fragment_for_prepared_instruction`, full terminator dispatch, function
  traversal, object-data handling, call emission, and local/global memory
  ownership remain outside the scalar module.
- Select/publication and broad dispatch remain separate. Existing scalar
  select helpers predate this runbook, while select-edge publication,
  predecessor publication movement, move-bundle assembly, and broad prepared
  instruction dispatch remain parked outside the moved scalar fragment set.
- The Step 2 and Step 3 proof records in this file and git history show the
  delegated build plus seven-test backend RV64/object subset passed after both
  the extraction and prune slices, covering the intended branch/object-byte
  preservation checks.
- No review blocker found for branch behavior, compare predicate
  normalization, branch-label fixups, diagnostics, emitted object bytes,
  gcc_torture expectations, unsupported markers, or named-case-only shortcuts.

Remaining parked helper work for later ideas:

- Select-edge publication and predecessor publication movement helpers.
- Before-return move bundles and full terminator/broad instruction dispatch.
- Call, local/global memory, object-data, formal-entry, variadic, inline asm,
  and saved-register helper families still shared by non-scalar routes.
- Shared append/load/store/move wrappers and scalar API declarations that still
  have live cross-module callers.

## Suggested Next

Ask the plan owner to evaluate closure of the active runbook.

## Watchouts

- Plan exhaustion is separate from source-idea closure; plan-owner owns the
  final lifecycle decision.
- The parked helper families above should move only under later source ideas
  that own those boundaries.
- Do not touch `review/global_address_helper_cleanup_review.md`.

## Proof

No build was required for this review-only packet. Review used the source idea,
active plan, accumulated git diff since plan activation, and the committed Step
2/Step 3 proof records in `todo.md`/git history.

Existing proof command recorded for both implementation slices:

```bash
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_prepared_fused_compare|codegen_route_riscv64_prepared_fused_compare|obj_runtime_rv64_return_add|obj_runtime_rv64_return_add_sub_chain)'" > test_after.log 2>&1
```

Recorded result: build succeeded; 7/7 selected tests passed after Step 2 and
again after Step 3. No new `test_after.log` was written for Step 4.
