Status: Active
Source Idea Path: ideas/open/443_closed_world_formal_pointer_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: residual disposition and close-readiness review for the
closed-world formal pointer authority carrier.

Rechecked facts:

- Internal/static authority carrier works: Step 3 added
  `bir::FormalPointerAuthorityKind`, populated `InternalOnly` from
  `LirFunction::is_internal`, and idea 442 consumes the authority field.
- External-linkage `930930-1::f` remains fail-closed. The refreshed prepared
  dump still has computed-address same-module call sources for `f`, but the
  four pointer-value rows remain `layout_authority=unknown` and
  `range_verdict=unknown_compatible`.
- `FormalPointerAuthorityKind::NoExternalCaller` is reserved as an accepted
  consumer state, but no producer populates it. Current lowering writes only
  `InternalOnly` or `Unknown` from `LirFunction::is_internal`.
- Pointer-delta propagation such as `%mr_TR - 8` remains out of scope until the
  base formal pointer has proven authority.

Artifacts:

- `build/agent_state/443_step4_residual_disposition/classification.md`
- `build/agent_state/443_step4_residual_disposition/evidence_snippets.txt`
- `build/agent_state/443_step4_residual_disposition/930930-1.prepared.out`
- `build/agent_state/443_step4_residual_disposition/930930-1.object.err`

## Suggested Next

Plan-owner close-readiness review.

Recommended lifecycle disposition: close idea 443 as the completed authority
carrier prerequisite, and split a separate producer idea if the project wants
to populate `NoExternalCaller` or an equivalent closed-world/no-external-caller
fact for non-internal definitions. Do not route external-linkage formal
provenance publication back to idea 442 until that producer exists.

## Watchouts

- Do not publish formal pointer provenance for external-linkage callees from
  observed same-module direct callsites alone.
- Do not treat hidden/protected visibility, same-module call wrappers,
  `LinkNameId`, or `can_elide_if_unreferenced` as no-external-caller authority
  without a separately defined and tested contract.
- `FormalPointerAuthorityKind::NoExternalCaller` is a reserved accepted
  consumer state, but no producer populates it yet.
- Do not mark `930930-1` done while `%lv.param.reg1` and `%mr_TR - 8` rows
  remain `layout_authority=unknown` and
  `range_verdict=unknown_compatible`.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Keep pointer-delta propagation behind proven base formal pointer authority.
- Do not accept or commit the rejected `test_baseline.new.log` full-suite
  baseline refresh with failing `string_authority_guard`.

## Proof

Step 4 delegated backend proof is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
