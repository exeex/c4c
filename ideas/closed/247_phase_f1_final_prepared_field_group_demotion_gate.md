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

## Closure Notes

Closed by the Phase F1 final prepared field-group demotion gate after the
prerequisite x86 Route 6, riscv Route 5/Route 3, and prepared
compatibility-retention ideas closed.

Lifecycle result: no `PreparedBirModule` or `PreparedFunctionLookups` field
group is safe for final deletion, demotion, or privatization from this gate.
No follow-up implementation ideas were created.

Draft 155 disposition: keep blocked with named blockers. The useful successor
work from ideas 243-246 proves selected route-native diagnostics, adapters,
compatibility-retention rows, and wrapper-output baselines, but it does not
prove whole prepared aggregate retirement.

Durable blockers:

- Public lookup groups such as `call_plans`, `memory_accesses`,
  `edge_publications`, and `edge_publication_source_producers` still lack full
  cross-target route-native positive, missing, mismatch, duplicate, fallback,
  wrapper-output, and baseline proof.
- Prepared compatibility/status surfaces remain intentionally retained:
  helper/oracle statuses, route-debug names, fallback names, and wrapper-output
  compatibility rows still depend on stable prepared naming.
- Target-policy groups for ABI, layout, register, stack, storage, addressing,
  carriers, runtime helpers, emission, formatting, and wrapper behavior remain
  target/prepared policy and are not BIR retirement candidates.
- `PreparedBirModule` public/control fields including `module`, `names`,
  `control_flow`, `call_plans`, and `store_source_publications` lack
  whole-route replacement proof and cannot be hidden behind private wrappers.
- Private pass context groups such as `route`, `invariants`,
  `completed_phases`, `notes`, and possibly `liveness` are not enough to
  justify implementation work from this gate.

Closure proof used matching `test_before.log` and `test_after.log` for:

```bash
cmake --build build-x86 --target backend_prepared_lookup_helper_test backend_x86_route_debug_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_x86_route_debug|backend_riscv_prepared_edge_publication)$'
```

Regression guard result: PASS, 3/3 before and 3/3 after, with no new failures.
