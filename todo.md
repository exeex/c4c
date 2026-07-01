Status: Active
Source Idea Path: ideas/open/478_semantic_instruction_result_write_event_carrier.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Write Event Carrier

# Current Packet

## Just Finished

Implemented the independent prepared semantic instruction-result frame-slot
write/materialization event carrier/status surface.

Added planner/checker API:

| Surface | Purpose |
| --- | --- |
| `PreparedSemanticWriteEventAuthorityKind` | Names explicit semantic write authority separately from `none` and unsupported authority. |
| `PreparedSemanticWriteEventCarrierStatus` | Distinguishes `available`, missing semantic identity, missing event authority, storage-only movement, semantic mismatch, destination mismatch, unsupported authority, and protected boundaries. |
| `PreparedSemanticWriteEventCarrierInputs` | Accepts explicit semantic result, producer identity, destination frame-slot/object facts, event site, event source, and event authority. |
| `plan_prepared_semantic_write_event_carrier` | Produces a carrier only from explicit event inputs; no raw-shape, final-home, or storage-order inference. |
| `prepared_semantic_write_event_carrier_available` | Availability predicate for the new carrier only. |

Focused backend coverage:

| Case | Verdict |
| --- | --- |
| Synthetic explicit semantic-result write to matching frame slot/object | `available`. |
| Missing semantic opcode/result identity | `missing_semantic_result_identity`. |
| Real `%t23`-style destination facts without explicit event authority | `missing_event_authority`. |
| `%t22 -> %t23` authority-none storage movement | `storage_only_move`. |
| Event source/result mismatch | `semantic_result_mismatch`. |
| Event destination slot/object mismatch | `destination_mismatch`. |
| Unsupported event authority | `unsupported_event_authority`. |
| Protected non-478 boundary | `unsupported_boundary`. |

Preserved boundaries:

- Real `%t23` is not marked available without explicit event authority.
- No path/no-clobber interval proof was added.
- No `PreparedFrameSlotSourceFact` population, branch-stack-load authority
  availability, or RV64 lowering was changed.
- No raw `.phi`/value-name/testcase-shape inference was introduced.

Changed files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`

Artifacts:

- `build/agent_state/478_step3_semantic_write_event_carrier/summary.md`

## Suggested Next

Execute Step 4 from `plan.md`: Residual Disposition And Close Readiness.
Re-probe or classify whether idea 478 is close-ready as an independent carrier
surface, and route any remaining work to the lower-level producer that can
publish real semantic write-event authority for `%t23` if needed.

## Watchouts

- The new carrier is an explicit-input surface only; it is not a collector for
  real prepared modules yet.
- Downstream interval/source-fact/branch authority/RV64 consumers must stay
  blocked unless a real event authority record exists and later path/no-clobber
  facts are proven.
- `%t22 -> %t23` `authority=none` remains storage movement, not semantic
  instruction-result materialization.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
