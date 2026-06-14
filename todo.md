Status: Active
Source Idea Path: ideas/open/259_phase_f3_x86_route5_prepared_edge_publication_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused Agreement and Rejection Proof

# Current Packet

## Just Finished

Step 4: `Focused Agreement and Rejection Proof` is complete.

Extended
`tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp` around
the existing `branch_join_loadlocal_then_add` EdgeStoreSlot selected-`LoadLocal`
fixture. The proof keeps prepared edge-publication rows produced by normal
preparation and drifts only the underlying BIR/Route 5 evidence before x86
emission.

Proved rows:

- positive selected `LoadLocal` Route 5 plus Route 3 agreement still emits the
  canonical x86 arm
- missing Route 5 source-memory evidence no longer accepts the agreed output
- Route 5/Route 3 source-memory offset mismatch no longer accepts the agreed
  output when only `LoadLocalInst::byte_offset` drifts and the Route 3
  `address.byte_offset` evidence stays unchanged
- Route 5 source-producer instruction-index mismatch no longer accepts the
  agreed output
- incomplete prepared source-memory authority still rejects through the
  canonical handoff error

The tightened offset-mismatch row exposed a narrow Step 3 wiring gap. Fixed
`src/backend/mir/x86/module/module.cpp` so x86 Route 3 `LoadLocal`
source-memory agreement compares the effective local load byte offset
(`LoadLocalInst::byte_offset + address.byte_offset`) against the prepared
frame-slot access before Route 5 accepts the selected publication. The proof
now changes only the `LoadLocalInst::byte_offset` component, leaving the Route
3 memory-address offset unchanged.

Rows not added here because no supported x86 fixture surface expresses them
without hand-building stale prepared publication state: duplicate Route 5
records for one natural edge, prepared-only/Route 5-only publication rows, and
wrong-edge publication rows. Existing missing-source-memory, producer mismatch,
and incomplete-source-memory rows cover the reachable selected-`LoadLocal`
fail-closed branch for this packet.

## Suggested Next

Step 5: run the compatibility sweep and closure-readiness proof. Recheck that
prepared lookup/status names, helper/oracle names, x86 output spelling,
route-debug output, RISC-V diagnostics, and non-`LoadLocal` compatibility
fallback behavior stayed unchanged.

## Watchouts

- `LoadLocal` edge-publication moves are the fail-closed branch; non-`LoadLocal`
  non-agreeing rows intentionally stay on the compatibility prepared-emission
  path.
- The new Route 5 drift tests assert that the canonical agreed output is not
  accepted after evidence drift. Some drift modes reject with the canonical
  handoff error; others suppress the publication move and produce non-canonical
  output.
- Avoid adding duplicate/wrong-edge rows by constructing stale prepared
  publication records by hand. Record unsupported fixture surfaces instead.
- `clang-format` is not installed in this environment; formatting was kept
  manually consistent with nearby code.

## Proof

Delegated proof command:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_riscv_prepared_edge_publication)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` shows 3/3 default
prepared/RISC-V tests and 2/2 x86-enabled tests passed.
