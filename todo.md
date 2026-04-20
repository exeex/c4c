# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2.3
Current Step Title: Remove Residual Scalar Home-Reconstruction Seams
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 3.2.2 (`Prove A Naturally Produced Or Rematerializable Home Path`) is now
covered by a non-mutated shared rematerializable lane in
`tests/backend/backend_x86_handoff_boundary_scalar_smoke_test.cpp`: the new
`const_add` fixture prepares `%sum` as a naturally produced
`PreparedValueHomeKind::RematerializableImmediate` home with payload `42`, and
the minimal scalar x86 consumer now reads that shared prepared home to emit the
canonical `mov eax, 42; ret` route without any test-only value-location
mutation. Focused `backend_x86_handoff_boundary` proof passed for the added
lane, and supervisor-side monotonic regression guard stayed flat against the
matching focused baseline.

## Suggested Next

Advance to Step 3.2.3 by removing the remaining scalar emitter-local home
reconstruction seams that still special-case register and stack sourcing inside
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, while keeping value-home
ownership shared and proving the cleaned route against the same bounded scalar
smoke family.

## Watchouts

- Do not grow x86-local matcher or ABI/home guessing shortcuts.
- Keep value-home and move-bundle ownership in shared prepare.
- Do not silently fold idea 61 addressing/frame work or idea 59 generic
  instruction selection into this route.
- Keep the new consumer surface keyed by existing prepared IDs and name-table
  lookups rather than widening into string-owned parallel state.
- Parameter value homes now need to mean the canonical entry ABI location for
  consumers, not the later regalloc-assigned destination register that
  `BeforeInstruction` bundles may target.
- The bounded stack-home proofs still mutate the prepared value-location
  contract inside the scalar smoke tests; a later Step 3.2 slice should prefer
  a naturally produced stack-backed or rematerializable scalar fixture if one
  can stay bounded.
- The new rematerializable scalar proof currently covers only the immediate-root
  no-parameter lane; Step 3.2.3 still needs to remove the remaining emitter
  seams that branch on register-vs-stack sourcing for parameter-backed scalar
  routes.
- The bounded immediate-binary stack-home lanes now cover `add`, `or`, `xor`,
  `mul`, `and`, `shl`, `lshr`, and `ashr`; the next honest proof gap is a
  naturally produced non-mutated stack-home or rematerializable scalar fixture
  that the shared producer can emit cleanly.
- `PreparedValueHomeKind::RematerializableImmediate` is declared in shared
  prepare but still does not appear to be produced by current shared producer
  code, so a rematerializable follow-up should confirm producer support before
  widening the x86 consumer.
- `PreparedMovePhase::BlockEntry` is currently inferred from shared
  `phi_...` move reasons, while call/result/return bundles come from
  destination-kind classification in shared prepare; keep any later phase
  refinement shared instead of pushing it into x86.
- Expect the delegated `^backend_` proof subset to continue showing the same
  four pre-existing failures already present in `test_before.log` unless this
  packet unexpectedly touches unrelated backend routes:
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary' > test_after.log 2>&1`,
which passed with the new Step 3.2.2 `const_add` rematerializable-home scalar
coverage. `test_after.log` is the canonical proof artifact for this packet.
Supervisor-side monotonic regression guard
(`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
--before test_before.log --after test_after.log
--allow-non-decreasing-passed`) passed with no new failing tests and no
pass-count regression.
