Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Design the Agreement Boundary

# Current Packet

## Just Finished

Completed Step 2: Design the Agreement Boundary for the idea 260 `module`
lookup-reader packet.

Chosen helper placement:

- Introduce one shared prealloc lookup helper used by both
  `prepared_lookups.cpp` and `select_chain_lookups.cpp` instead of carrying
  two divergent file-local `prepared_bir_function(...)` /
  `prepared_bir_block_label_id(...)` policies.
- Keep the helper under the prealloc lookup layer, with the public surface
  limited to the selected reader row. Do not move ownership into `bir`,
  route/debug code, backend module lowering, printer code, or a broad
  `PreparedBirModule` wrapper.
- `prepared_lookups.cpp` should also use the shared block-label agreement
  helper from `prepared_bir_block(...)`; `select_chain_lookups.cpp` should use
  the same helper when publishing source-producer block labels.

Accepted agreement rows:

- Function agreement is accepted only when the prepared function id is valid,
  its prepared spelling is non-empty, exactly one BIR function has the same
  display name, and the selected helper is being used for the current
  prepared control-flow function.
- Block agreement is accepted only when the prepared block label id is valid,
  its prepared spelling is non-empty, the BIR block has a valid structured
  `label_id`, that BIR id spells the same non-empty label in
  `prepared.module.names.block_labels`, and that spelling resolves to the same
  prepared block label id.
- Block-label-id agreement is accepted only when the BIR structured
  `label_id` maps through `prepared.module.names.block_labels` to a non-empty
  spelling and that spelling maps through `prepared.names.block_labels` to the
  same intended prepared label id.
- Existing prepared lookup results remain authoritative when they already
  have a valid answer; the new structure fact only claims agreement for the
  null or invalid structured path named by this packet.

Rejected fail-closed rows:

- Reject `kInvalidFunctionName`, empty prepared function spelling, absent BIR
  function, duplicate same-name BIR functions, and prepared/BIR function-name
  mismatch by returning the current null or empty lookup result.
- Reject `kInvalidBlockLabel`, empty prepared block spelling, invalid BIR
  `label_id`, empty BIR structured label spelling, missing prepared label id,
  mismatched prepared label id, duplicate/conflicting BIR blocks for the same
  prepared label, and prepared/BIR label drift by returning the current null
  or `kInvalidBlockLabel` result.
- Reject raw-label-only agreement as a structure fact: raw `block.label`
  fallback remains compatibility behavior, not evidence that the BIR
  structured label agreed with prepared ids.
- Preserve existing null/fallback behavior for missing prepared ids, stale BIR
  labels, label-string fallback, absent control-flow/module structure, and
  invalid labels; do not rewrite printer/debug, route-debug, target output,
  status names, or unsupported expectations to claim progress.

Candidate implementation and test files:

- Implementation files:
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/select_chain_lookups.cpp`, and either
  `src/backend/prealloc/prepared_lookups.hpp` for a narrow shared declaration
  or a new small prealloc lookup helper pair if the implementation needs to
  avoid exposing the helper through the aggregate lookup header.
- Focused tests:
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Stability proof surfaces:
  `tests/backend/bir/backend_prepared_printer_test.cpp`,
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`, and
  the x86 route-debug/handoff bucket selected by the supervisor.

## Suggested Next

Execute Step 3 from `plan.md`: implement the narrow lookup-reader bridge.

Suggested Step 3 implementation packet:

- Objective: implement the shared agreement helper for the idea 260 `module`
  lookup-reader packet and wire it into the current helper readers without
  changing public prepared aggregate compatibility.
- Owned files:
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/select_chain_lookups.cpp`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and
  `todo.md`; add a narrow prealloc helper header/source only if sharing the
  agreement helper cleanly requires it.
- Do not touch:
  `plan.md`, `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`,
  unrelated `PreparedBirModule` fields, printer/debug baselines, route-debug
  strings, target-output expectations, unsupported expectations, or other
  idea 260 candidates.
- Implement:
  one shared helper for function identity and one shared helper for structured
  block-label agreement; use them from both lookup construction paths. Keep
  raw label fallback as compatibility-only behavior after agreement fails,
  and do not count fallback as accepted BIR structure authority.
- Add focused rows proving a positive structured-id agreement for function and
  block lookup, plus rejection for invalid prepared ids, invalid BIR ids,
  missing prepared names, stale structured BIR labels, raw-label-only fallback,
  duplicate/conflicting labels, and prepared/BIR label drift.
- Done when:
  build and focused tests pass, `todo.md` records Step 3 completion, and the
  diff contains no output baseline rewrites, unsupported downgrades, or
  unrelated candidate movement.

Supervisor-selected proof command for Step 3:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_riscv_prepared_edge_publication)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

## Watchouts

- Keep this runbook limited to the selected `module` lookup-reader candidate.
- Do not treat the activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve existing null, fallback, `kInvalidBlockLabel`, stale-label, and
  prepared/BIR drift behavior.
- Do not rewrite printer/debug, route-debug, target output, baselines, or
  unsupported expectations to claim progress.
- The current block helper has raw-label fallback after structured label-id
  miss; Step 2 must explicitly decide which rows may use the new BIR structure
  fact and which rows must retain null or `kInvalidBlockLabel` results.
- The selected row has duplicate helper logic in `prepared_lookups.cpp` and
  `select_chain_lookups.cpp`; avoid fixing one path while leaving the other
  with a contradictory agreement policy.
- Keep the public prepared aggregate and existing prepared lookup maps
  observable. This packet is not authorization to delete fields, privatize the
  aggregate, wrap the module, or migrate another `module`, `names`,
  `control_flow`, or `store_source_publications` candidate.

## Proof

Ran:

```bash
git diff --check -- todo.md
```

Result: passed. This packet is design-only; no build/test proof was needed
because no implementation or tests changed.
