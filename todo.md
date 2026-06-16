Status: Active
Source Idea Path: ideas/open/290_gate_quarantined_opaque_compatibility_memory_accesses.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Quarantined Compatibility Consumers

# Current Packet

## Just Finished

Activation created the active runbook from the source idea. No executor packet
has completed yet.

## Suggested Next

Begin `Step 1: Inventory Quarantined Compatibility Consumers` by mapping
prepared and target-independent consumers that still accept
`OpaqueCompatibility` or `UnknownCompatible` source-memory rows, identifying
candidate policy surfaces, and naming the focused proof subset required by
`plan.md`.

## Watchouts

- Do not gate on `UnknownCompatible` or `can_use_base_plus_offset` without
  checking explicit `layout_authority == OpaqueCompatibility` or an equivalent
  flattened opaque-compatibility fact.
- Do not weaken stale-row, duplicate-row, or cross-block prepared
  `memory_accesses` rejection.
- Do not reject structured proven in-range rows by conflating quarantine
  metadata with general unknown extent.
- Do not use target-specific prepared/prealloc code as the main policy surface
  while target-independent provenance facts are available.

## Proof

Not run; lifecycle activation only.
