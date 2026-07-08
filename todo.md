Status: Active
Source Idea Path: ideas/open/607_destination_fan_in_authority_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Document Current Failure Shapes

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: created `docs/destination_fan_in_authority/01_current_failure_shapes.md` documenting the current non-parallel multi-source stack-destination failure shapes before any destination authority rule is selected.

Selected representative rows:

- `src/pr43236.c`: failure-map representative for the `125` row family; detailed per-case log at `build/rv64_gcc_c_torture_backend/src_pr43236.c/case.log`.
- `src/20000113-1.c` and `src/20040409-1w.c`: July 8 detailed logs showing entry/early-instruction two-register fan-in to one stack destination.
- `src/20021010-2.c` and `src/20021120-3.c`: current downstream logs with the same `ambiguous_non_parallel_multi_source_stack_destination` diagnostic shape.
- `src/pr48814-2.c`: representative short classifier rejection for the same prepared consumer category.

Evidence note: `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md` remains the authoritative planned `125` row family count for this research. A raw grep of the current per-case workdir finds `128` logs with `ambiguous_non_parallel_multi_source_stack_destination`; the Step 1 document treats those extra current-log hits as representative shape evidence, not a count rewrite.

## Suggested Next

Proceed to Step 2: decide whether the destination legality rule should be ordering, mutual exclusion, merge authority, or explicit rejection, using `01_current_failure_shapes.md` as the failure-shape boundary.

## Watchouts

- Keep this route documentation-only.
- Do not change implementation files, tests, expectations, unsupported markers, allowlists, runtime behavior, timeout policy, or accounting.
- Do not select a destination by testcase shape, source order accident, or RV64 assembly convenience.
- Keep source freshness authority separate from destination fan-in authority.
- Do not let the raw current-workdir `128` grep count silently replace the recovery map's `125` planned family count without supervisor/plan-owner direction.
- Preserve exactly one active source idea: `ideas/open/607_destination_fan_in_authority_research.md`.

## Proof

Documentation-only proof:

- `test -f docs/destination_fan_in_authority/01_current_failure_shapes.md`
- `rg -n 'build/rv64_gcc_c_torture_backend|unsupported_prepared_move_bundle_classification|diagnostic_owner|prepared_consumer_category' docs/destination_fan_in_authority/01_current_failure_shapes.md`
- `git diff --check -- docs/destination_fan_in_authority/01_current_failure_shapes.md todo.md`

No root-level proof logs were created or modified for this documentation-only packet.
