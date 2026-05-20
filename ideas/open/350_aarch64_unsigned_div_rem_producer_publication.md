# AArch64 Unsigned Div Rem Producer Publication

Status: Open
Created: 2026-05-20
Split From: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md

## Goal

Repair AArch64 scalar producer publication for unsigned division and remainder
values so `udiv`/`urem` results are materialized into the registers or storage
consumed by later scalar operations and selected stores.

## Why This Exists

Idea 348 repaired selected aggregate address/writeback representatives
`00130`, `00187`, and `00195`. The remaining `00182` runtime mismatch now
reaches selected global stores for the static digit array, but the value being
stored comes from unsigned remainder/truncation, and the loop update consumes
an unsupported unsigned division producer. Generated assembly reuses stale
condition or scratch register state instead of publishing the unsigned
div/rem producers before their consumers.

That first bad fact is scalar ALU producer publication, not dynamic selected
aggregate address/writeback.

## In Scope

- Localize unsigned division and remainder producer lowering in the AArch64
  path from prepared scalar operation through emitted result publication.
- Repair publication of `udiv` and unsigned remainder results to the scalar
  value consumed by truncation, stores, loop updates, comparisons, or selected
  aggregate stores.
- Add focused backend coverage for unsigned div/rem producers feeding ordinary
  scalar consumers and selected store values.
- Use `00182` as external proof only after focused coverage identifies the
  repaired producer-publication owner.

## Out Of Scope

- Dynamic indexed aggregate selected-address/writeback repairs already owned
  by idea 348.
- Recursive call argument preservation for `00176` and `00181`.
- Signed division/rem lowering, multiplication, logical shift, boolean or
  comparison materialization, FP expression lowering, semantic `lir_to_bir`
  admission, frame-slot layout, or aggregate call-boundary publication unless
  fresh evidence reaches those exact boundaries.
- Expectation changes, unsupported-classification changes, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- Fixing only `00182`, one temporary name, one digit buffer, one emitted
  register, or one instruction sequence.

## Acceptance Criteria

- The first bad fact is localized to a concrete unsigned div/rem scalar
  producer, result materialization, or consumer handoff boundary.
- Focused backend coverage fails without the repair and passes with it for
  unsigned division and/or remainder values feeding later scalar consumers.
- `00182` advances past the stale unsigned div/rem producer publication
  failure or is reclassified by a new first bad fact after repair.
- Supervisor-selected adjacent scalar publication and selected-store guardrails
  remain stable.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00182`, the LED digit array, one temporary such as `%t14`, one
  register, or one emitted instruction sequence instead of repairing general
  unsigned div/rem producer publication;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, diagnostic rewrites, classification
  notes, or output-count changes without making generated consumers read the
  actual unsigned div/rem result;
- broadens into recursive call preservation, selected aggregate address
  writeback, signed div/rem, multiplication, logical shift, or semantic
  admission without fresh first-bad-fact evidence;
- leaves generated AArch64 able to reuse stale condition or scratch state where
  a consumer should observe an unsigned division or remainder result.
