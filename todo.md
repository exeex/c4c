Status: Active
Source Idea Path: ideas/open/444_no_external_caller_formal_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit No-External-Caller Authority Inputs

# Current Packet

## Just Finished

Completed Step 1: audited no-external-caller authority inputs for
non-internal formal pointer provenance.

Authority table:

| Candidate | Classification | Reason |
| --- | --- | --- |
| `LirFunction::is_internal` / HIR `linkage.is_static` | Accepted existing authority | Already implemented by idea 443 as `FormalPointerAuthorityKind::InternalOnly`; this remains the only sound complete-caller authority currently produced. |
| `FormalPointerAuthorityKind::NoExternalCaller` | Accepted target carrier, missing producer | The enum and consumer predicate exist, but no frontend/HIR/LIR/prepared producer writes it. |
| Observed same-module direct calls | Rejected insufficient signal | They identify call observations only; external-linkage definitions can still be called from outside the module. |
| `LinkNameId`, same-module definition, non-declaration status | Rejected insufficient signal | These identify symbols and local definitions, not caller-set completeness. |
| Hidden/protected/default visibility | Rejected insufficient signal for this packet | Visibility is present in HIR/LIR text but has no tested compiler/linker contract here equating it to no external caller. |
| `can_elide_if_unreferenced` / dead-internal reachability | Rejected insufficient signal | The pass removes discardable static/inline/helper functions; it is not a durable no-external-caller proof for non-internal definitions. |
| External declaration absence | Rejected insufficient signal | A defined external-linkage function need not have a separate declaration in the same module, and outside callers remain possible. |
| Function-address/global-initializer references | Missing/incomplete authority | Existing metadata helps preserve/materialize addresses but does not provide complete function-address escape or indirect-call target exclusion. |

Representative result:

- `930930-1::f` remains a rejected external-linkage/no-proof case.
- The refreshed prepared dump records computed-address same-module call sources
  to `f`, including `source_base=@mem source_delta=792`.
- Pointer-value rows through `%p.reg1` and `%mr_TR - 8` still have
  `layout_authority=unknown` and `range_verdict=unknown_compatible`.

Missing metadata needed before any `NoExternalCaller` producer can be sound:

- durable closed-world/private/no-external-caller function authority for
  non-internal definitions;
- complete caller-set proof for external-linkage definitions;
- module-level function-address escape and indirect-call target exclusion
  facts;
- a tested visibility/linker contract if hidden/protected/private visibility is
  ever used as authority.

Artifacts:

- `build/agent_state/444_step1_no_external_caller_audit/audit.md`
- `build/agent_state/444_step1_no_external_caller_audit/representative_snippets.txt`
- `build/agent_state/444_step1_no_external_caller_audit/930930-1.prepared.out`
- `build/agent_state/444_step1_no_external_caller_audit/930930-1.hir_summary.out`

## Suggested Next

Execute Step 2: Define The Producer Contract.

Recommended Step 2 packet: write the precise `NoExternalCaller` contract and
record that the current pipeline has no implementation-ready producer for
non-internal definitions. The contract should keep `NoExternalCaller` false by
default unless a future source provides tested closed-world/no-external-caller
metadata with address-escape and indirect-call coverage.

## Watchouts

- Do not publish formal pointer provenance for external-linkage callees from
  observed same-module callsites alone.
- Do not treat hidden/protected visibility, same-module wrappers, `LinkNameId`,
  or `can_elide_if_unreferenced` as no-external-caller authority without a
  tested contract.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Keep `%mr_TR - 8` pointer-delta propagation behind proven base formal pointer
  authority.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 1 classification-only check:

```sh
git diff --check
```
