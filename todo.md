Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation and Closure Readiness

# Current Packet

## Just Finished

Completed Step 5: Broader Validation and Closure Readiness for the idea 260
selected `module` lookup-reader candidate.

The selected candidate is complete under this runbook:

- the shared prealloc lookup agreement helper is implemented and used by the
  selected prepared lookup-reader paths
- positive and fail-closed proof rows cover the structured agreement boundary,
  stale or missing structured ids, duplicate/conflicting labels, absent module
  structure, prepared/BIR drift, and legacy raw-label compatibility surfaces
- public prepared aggregate fields and existing prepared lookup maps remain
  observable
- no other idea 260 candidate was absorbed into this plan

The selected candidate is ready for plan-owner closure review.

## Suggested Next

Request plan-owner closure review for idea 260. The active runbook should close
or be replaced rather than expanding in place to another idea 260 candidate.

## Watchouts

- Keep this runbook limited to the selected `module` lookup-reader candidate.
- Do not treat the activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve existing null, fallback, `kInvalidBlockLabel`, stale-label, and
  prepared/BIR drift behavior.
- Do not rewrite printer/debug, route-debug, target output, baselines, or
  unsupported expectations to claim progress.
- The shared helper still allows legacy fixtures whose BIR `label_id` resolves
  through the prepared name table instead of the module name table; unknown
  ids remain conflicts, not raw-label fallback.
- Keep the public prepared aggregate and existing prepared lookup maps
  observable. This packet is not authorization to delete fields, privatize the
  aggregate, wrap the module, or migrate another `module`, `names`,
  `control_flow`, or `store_source_publications` candidate.
- Empty prepared function/block spelling is represented by invalid ids in this
  fixture. Forcing a separate empty-spelling row would require mutating
  name-table internals and stays out of scope for this packet.
- Residual out-of-scope rows remain outside this runbook: `PreparedBirModule`
  deletion, aggregate privatization or wrapping, broad module/name/control-flow
  migration, and any other idea 260 candidate.

## Proof

Broader delegated proof:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` shows 180/180 default backend tests and 2/2
x86 route-debug/handoff tests passed.
