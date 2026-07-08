Status: Active
Source Idea Path: ideas/open/607_destination_fan_in_authority_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Document Current Failure Shapes

# Current Packet

## Just Finished

Activated Step 1 from `plan.md`: document the current non-parallel multi-source stack-destination failure shapes before any destination authority rule is selected.

## Suggested Next

Inspect the July 8 RV64 gcc_torture scan artifacts and representative per-case logs, then create `docs/destination_fan_in_authority/01_current_failure_shapes.md` with the row table, diagnostic vocabulary, prepared/prealloc surfaces, and row-family boundary.

## Watchouts

- Keep this route documentation-only.
- Do not change implementation files, tests, expectations, unsupported markers, allowlists, runtime behavior, timeout policy, or accounting.
- Do not select a destination by testcase shape, source order accident, or RV64 assembly convenience.
- Keep source freshness authority separate from destination fan-in authority.
- Preserve exactly one active source idea: `ideas/open/607_destination_fan_in_authority_research.md`.

## Proof

No validation run for activation; lifecycle-only plan/todo creation.
