# Current Packet

Status: Active
Source Idea Path: ideas/open/722_direct_edge_publication_available_move_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair general producer and admission continuity

## Just Finished

- Plan Step 2 repaired both general out-of-SSA move producers. Named-source moves in `regalloc/move_records.cpp` and immediate-source phi moves in `regalloc/phi_moves.cpp` now copy the already-owned assigned register spelling into `PreparedMoveResolution::destination_register_name`; duplicate identity checks use the same optional spelling.
- Stack destinations still publish no register spelling, while register destinations now keep storage kind, placement, and spelling together at the earliest producer boundary. No typed-view inference, target fallback, Route 3/Route 5 logic, or x86 module change was added.
- Natural semantic producer coverage now proves both paths: `backend_prepare_liveness` checks a named-source phi register destination against its regalloc assignment and separately requires stack destinations to omit register spelling; `backend_prepare_phi_materialize` checks an immediate-source phi register destination against its prepared home. Neither test injects a `PreparedMoveResolution`.

## Suggested Next

- Accept this producer-only Step 2 slice, then handle the remaining semantic-fixture continuity gap separately: the joined-branch boundary test still performs a post-prepare home-only mutation, so it cannot exercise the repaired producer record without a genuine pre-regalloc semantic allocation input.

## Watchouts

- The selected baseline already contains three failures: typed core comparator source authority, decoded-home-storage source authority, and the joined-branch home-only mutation. This producer patch does not add failures; `backend_prepared_lookup_helper` remains green.
- The joined-branch fixture's post-prepare mutation is intentionally not synchronized by producer code: repairing it downstream would turn stale prepared state into authority. A distinct semantic input capable of selecting the desired registers before regalloc remains necessary for that surface.

## Proof

- Ran `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_decoded_home_storage|backend_prepared_mir_core_comparator|backend_prepared_lookup_helper)$' >> test_after.log 2>&1`.
- Build passed. Results match `test_before.log`: `backend_prepared_lookup_helper` passed; the same three pre-existing authority/fixture failures remain. `test_after.log` is the canonical proof log.
- Ran `ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_phi_materialize)$'`; both directly modified producer tests passed.
