Status: Active
Source Idea Path: ideas/open/166_bir_annotation_lookup_index_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Route Index Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1 of
`ideas/open/166_bir_annotation_lookup_index_schema.md`.

## Suggested Next

Execute Step 1: inventory existing Route 1 through Route 7 BIR index helpers,
lookup keys, typed record references, private aggregate facades, divergence
checks, positive/negative coverage, and the narrow proof command for the first
code-changing packet.

## Watchouts

- Keep typed Route 1 through Route 7 annotation records as the semantic source
  of truth.
- Do not create a durable BIR-owned `PreparedFunctionLookups` clone.
- Do not add ABI, layout, register allocation, move scheduling, call plan,
  memory-addressing, frame, dynamic-stack, helper, final-emission,
  storage-home, or target lowering policy data to function-local indexes.
- Do not duplicate semantic payload fields in indexes when typed annotations
  already own those payloads.
- Preserve explicit divergence, stale-reference, duplicate-key, missing-record,
  wrong-relationship, and no-match cases where applicable.

## Proof

Activation only. No build or test proof required.
