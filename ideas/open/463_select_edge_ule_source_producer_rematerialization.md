# Select-Edge ULE Source-Producer Rematerialization

Status: Open
Type: RV64 select-edge source-producer rematerialization / route-classification idea
Parent: `ideas/closed/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md`
Source Evidence: `build/agent_state/462_step4_residual_disposition/`
Owning Layer: RV64 predecessor-edge source-producer rematerialization for select/phi carriers

## Goal

Classify and, if justified, implement the source-producer rematerialization
route for the proven `%t46 -> %t50` predecessor-edge select result residual:
`%t46 = bir.ule ptr %t42, %t45` is produced in successor block
`logic.end.41`, feeds `%t50.phi.sel0` / `%t50.phi.sel1`, and is required for
the `logic.rhs.end.40 -> logic.end.41` edge.

## Why This Exists

Idea 462 proved that the coordinate-bearing `pre_terminator_copies` /
`out_of_ssa_parallel_copy` event is not a sound plain predecessor-edge GPR
copy. `%t46` is defined in successor block `logic.end.41`, after predecessor
block `logic.rhs.end.40` branches there, so treating its prepared GPR home as
predecessor-available would infer availability from value homes alone. The
remaining owner is the source-producer route that can decide whether `%t46`
can be rematerialized at the predecessor edge from `%t42` and `%t45`.

## In Scope

- Audit why the existing select-edge source-producer consumer does not
  rematerialize `%t46 = bir.ule ptr %t42, %t45` for destination `%t50` on edge
  `logic.rhs.end.40 -> logic.end.41`.
- Classify duplicate carrier facts `%t50.phi.sel0` / `%t50.phi.sel1` and
  whether they are valid carrier-only uses, ambiguous duplicates, or a
  producer/prepared metadata blocker.
- Audit operand availability for `%t42` and `%t45` at the predecessor edge,
  including value homes, pointer authority, layout/provenance authority, and
  target-consumable RV64 forms.
- If justified, implement the narrow rematerialization route for this
  coordinate/authority-backed unsigned pointer relational source producer with
  focused fail-closed coverage.
- Route any missing operand authority, duplicate-carrier ambiguity, or
  unrelated move-bundle/storage residuals to separate ideas instead of
  broadening this one.

## Out Of Scope

- Plain preterminator predecessor-edge register copies for successor-defined
  values.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Treating successor-block values as predecessor-available from prepared value
  homes alone.
- Reopening ideas 456, 458, 459, 460, 461, or 462 without new
  coordinate-bearing evidence that their exact route owns the first failure.
- Testcase-name, function-name, block-label, value-id-only, raw BIR-shape, or
  prepared-dump-order matching.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- The `%t46 -> %t50` select-edge source-producer route is classified with an
  accepted rematerialization contract or a precise blocker.
- The classification explains duplicate `%t50.phi.sel0` / `%t50.phi.sel1`
  carrier facts and operand availability for `%t42` / `%t45` at
  `logic.rhs.end.40 -> logic.end.41`.
- Any implementation consumes explicit source-producer, edge, carrier, and
  operand-authority facts and remains fail-closed for missing, ambiguous,
  duplicate-unproven, non-edge, non-consumable, stale stack-load, and generic
  move cases.
- Fresh residual disposition records whether `20010329-1` advanced and routes
  any next first-owner without broadening this source idea.

## Reviewer Reject Signals

- Reject a plain `%t46 -> %t50` copy or same-register no-op that treats
  successor-defined `%t46` as predecessor-available from value homes alone.
- Reject generic move-bundle support claimed as progress for this select-edge
  source-producer route.
- Reject testcase-shaped handling keyed on `20010329-1`, `logic.rhs.end.40`,
  `logic.end.41`, `%t46`, `%t50`, value ids, or block indexes without using
  prepared source-producer/edge/carrier/operand authority.
- Reject consuming stale stack-load authority or reopening the older
  cast-dependency/suppression/plain-copy routes without new coordinate
  evidence.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, baseline/log churn, or classification-only
  changes presented as implementation progress.
