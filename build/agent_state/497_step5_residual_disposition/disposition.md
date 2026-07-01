# 497 Step 5 - Residual Disposition

## Decision

Idea 494 cannot resume available interval fact publication yet.

The first remaining owner is still the lower endpoint/effect owner for ordered
effect-source stream population. Step 3 supplied bounded endpoint bridge
records, and Step 4 classified the blocker instead of accepting synthetic
coverage, but production still lacks a builder that owns the selected
proof-source-to-endpoint interval and emits comparable ordered effect-source
records from prepared/BIR sources.

## Handback State

- Endpoint bridge: available for supported rows through
  `bir::LocalArrayEndpointBridgeRecord`.
- Selected proof-source coordinates: present through the selected proof-edge
  path surface.
- Ordered effect-source stream: missing in production.
- Available no-clobber publication: not ready.

## Required Before Idea 494 Resumes Availability

The lower owner must populate ordered effect-source records for the selected
proof-source-to-endpoint interval, covering assignments/redefinitions, memory
accesses, phi/alias transfers, calls/helpers, inline asm, publications, move
bundles, parallel copies, and unknown modeled effects.

The interval classifier should then consume the builder-backed bounded scan
record without requiring the legacy `endpoint_bridge_available` compatibility
boolean as a separate availability source.

Until that exists, idea 494 should continue to publish precise fail-closed
statuses rather than available same-value/no-clobber facts.
