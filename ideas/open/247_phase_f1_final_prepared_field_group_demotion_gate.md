# 247 Phase F1 final prepared field-group demotion gate

## Goal

Decide whether any `PreparedBirModule` or `PreparedFunctionLookups` field
group has become safe for final demotion, deletion, or privatization after the
x86 Route 6, riscv Route 5/Route 3, and prepared compatibility-retention
packets have closed.

## Why This Exists

Phase F0 found that no whole prepared lookup group and no whole prepared field
group is deletion-ready today. The only safe final packet is a future gate that
rechecks field groups after route-native x86/riscv diagnostics, adapters, and
compatibility retention are proven. This idea records the deletion criteria
without pretending a deletion candidate is ready now.

## In Scope

- Rechecking `PreparedBirModule` and `PreparedFunctionLookups` field groups
  after the prerequisite F1 ideas close.
- Classifying each field group as duplicate semantic cache, private pass
  context, target-policy product, compatibility adapter, blocked public
  authority, or deletion candidate.
- Producing one follow-up implementation idea per safe final field-group
  demotion/deletion, if any exists.
- Recording that no field group is safe to delete when proof remains blocked.
- Deciding whether draft 155 should be rewritten, superseded, kept blocked, or
  retired obsolete.

## Out Of Scope

- Directly deleting or privatizing a field group in this gate.
- Opening draft 155 as broad prepared aggregate retirement without complete
  prerequisite proof.
- Treating x86-only, riscv-only, diagnostic-only, or compatibility-only proof
  as whole-aggregate readiness.
- Moving target policy into BIR or hiding prepared public authority behind new
  private wrapper names.
- Expectation weakening, baseline-only claims, or helper/oracle renames as
  proof of deletion readiness.

## Acceptance Criteria

- The gate names every remaining public prepared lookup group and
  `PreparedBirModule` field group relevant to Route 3, Route 4, Route 5, Route
  6, wrappers, diagnostics, fallback, and target policy.
- Any proposed deletion/demotion candidate has cross-target route-native
  positive, missing, mismatch, duplicate, fallback, wrapper-output, and
  baseline proof, or is explicitly scoped as target-local private pass context.
- Prepared aggregate retirement criteria are stated field-by-field, not as a
  broad "BIR owns semantics" claim.
- Draft 155 disposition is one of: rewrite with narrow field-group packets,
  supersede with smaller ideas, keep blocked with named blockers, or retire
  obsolete because all successor packets cover its useful scope.
- If no field group is safe, the gate closes with a blocker map rather than an
  implementation packet.

## Proof Requirements

- Fresh inspection of current `PreparedBirModule`, `PreparedFunctionLookups`,
  wrapper, diagnostic, helper-oracle, fallback, and baseline consumers.
- Evidence that prerequisite x86 Route 6, riscv Route 5/Route 3, and prepared
  compatibility-retention ideas are closed or explicitly named as blockers.
- Matching before/after regression guard for any lifecycle or docs-only close,
  and full backend or broader validation for any future implementation packet
  generated from this gate.
- A reviewer-ready map explaining why each candidate is deletion-ready,
  demotion-ready, private-pass-context-only, target-policy-owned,
  compatibility-owned, or blocked.

## Reviewer Reject Signals

- The gate opens deletion work while x86/riscv parity, route-native
  diagnostics, fallback, public prepared compatibility, or wrapper-output proof
  remains missing.
- The slice claims whole `PreparedBirModule`, `PreparedFunctionLookups`, or
  draft 155 retirement from selected one-reader adapters.
- Public authority is hidden behind renamed private helpers instead of
  removed, demoted, or retained with a clear compatibility contract.
- Target ABI, layout, register, stack, emission, formatting, or wrapper policy
  is moved into BIR as part of a deletion claim.
- Expectations, helper names, supported-path contracts, wrapper output, or
  baselines are weakened to make a field group appear unused.
