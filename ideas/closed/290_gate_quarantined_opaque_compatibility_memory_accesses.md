# Gate Quarantined Opaque Compatibility Memory Accesses

## Goal

Define and implement the next behavior-changing policy for prepared memory
access rows whose structured provenance remains quarantined as
`OpaqueCompatibility` / `UnknownCompatible`.

## Why This Exists

Idea 289 replaced the opaque pointer byte-offset compatibility bridge with
structured provenance facts and explicit quarantine metadata while preserving
existing opaque byte-addressed behavior. That slice deliberately stopped short
of rejecting or gating every remaining quarantined row, because changing
consumer acceptance policy requires choosing a concrete surface and proving
that route coverage is complete enough to fail closed safely.

## In Scope

- Inventory prepared and target-independent consumers that still accept
  `OpaqueCompatibility` / `UnknownCompatible` source-memory rows.
- Choose one explicit policy surface for the first behavior-changing step:
  lowerer-time rejection of quarantined addressed pointer load/store rows, or a
  prepared/publication consumer gate.
- If gating in prepared/publication consumers, use
  `layout_authority == OpaqueCompatibility` directly or publish an equivalent
  flattened fact and include it in stale-row matching.
- Add focused before/after tests for the selected surface, including accepted
  structured-proof rows, rejected or gated quarantined rows, and stale-row
  mismatch cases.
- Run broader backend/prepared validation appropriate for behavior-changing
  acceptance policy.

## Out of Scope

- Replacing structured provenance carrier fields introduced by idea 289.
- Treating `UnknownCompatible` alone as proof that a row came from the opaque
  pointer bridge.
- Target-specific acceptance or rejection that bypasses target-independent
  source-memory provenance facts.
- Broad prepared/prealloc rewrites unrelated to quarantined opaque
  compatibility policy.

## Acceptance Criteria

- The selected policy surface rejects, gates, or diagnoses quarantined opaque
  compatibility rows using explicit `OpaqueCompatibility` metadata rather than
  `can_use_base_plus_offset` or `UnknownCompatible` alone.
- Structured proven in-range rows remain accepted.
- Stale-row, duplicate-row, and cross-block prepared `memory_accesses`
  rejection remains at least as strict as the idea 289 implementation.
- Focused tests prove the chosen policy surface, and broader backend/prepared
  validation shows no regression in supported structured-provenance routes.

## Completion Notes

- Closed after selecting and implementing the lowerer-time rejection policy for
  quarantined addressed pointer load/store rows.
- The gate uses explicit `OpaqueCompatibility` metadata rather than
  `UnknownCompatible` or `can_use_base_plus_offset` alone.
- Structured proven in-range rows remain accepted, and focused regression
  coverage includes load-side and store-side opaque compatibility fail-closed
  fixtures.
- Close validation reused the supervisor-selected backend/prepared scope:
  `backend_lir_to_bir_notes`, `backend_prepared_lookup_helper`,
  `backend_riscv_prepared_edge_publication`, and `backend_prepared_printer`.
  The canonical close comparison passed 4/4 before and 4/4 after with no new
  failures.

## Reviewer Reject Signals

- The patch gates on `UnknownCompatible` or `can_use_base_plus_offset` without
  checking explicit `layout_authority == OpaqueCompatibility` or an equivalent
  flattened opaque-compatibility fact.
- The route downgrades supported-path expectations, weakens stale-row matching,
  or accepts a testcase-shaped exception to keep one known case green.
- Target-specific prepared/prealloc code is used as the main policy surface
  while target-independent provenance facts remain available but ignored.
- Structured proven in-range rows regress or are rejected because quarantine
  metadata is conflated with general unknown extent.
- The work mainly renames helpers, rewrites expected output, or changes
  classification text without enforcing a concrete reject/gate/diagnostic
  policy.
