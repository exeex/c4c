Status: Active
Source Idea Path: ideas/open/443_closed_world_formal_pointer_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Closed-World Formal Authority Inputs

# Current Packet

## Just Finished

Completed Step 1: audited current linkage, visibility, callgraph, declaration,
BIR, call-source, and prepared metadata surfaces for closed-world formal
pointer authority.

Authority table:

| Signal | Classification | Reason |
| --- | --- | --- |
| `LirFunction::is_internal` from static linkage | Accepted existing authority | HIR-to-LIR sets it from `fn.linkage.is_static`; idea 442 already consumes this safely. |
| Observed same-module direct callsites all passing coherent computed addresses | Rejected insufficient signal | Proves only observed call edges, not that an external-linkage definition has no outside callers. |
| `PreparedCallWrapperKind::SameModule` | Rejected insufficient signal | Classifies one prepared call edge, not the callee's complete caller set. |
| `CallArgumentSourceRelationship::ComputedAddress` | Rejected insufficient signal by itself | Describes one argument source, not formal incoming-value completeness. |
| `LinkNameId` / `FunctionSymbolSet` membership | Rejected insufficient signal | Semantic identity and known-symbol presence do not imply closed-world authority. |
| `bir::Function::is_declaration == false` | Rejected insufficient signal | A definition can still be externally callable; `930930-1::f` remains the representative blocker. |
| Hidden/protected/default visibility | Rejected for this packet | Current producer contracts do not prove visibility alone means no arbitrary external caller in this compilation model. |
| `LirFunction::can_elide_if_unreferenced` | Rejected insufficient signal | Includes inline/helper discardability beyond static linkage; it is not formal pointer authority. |
| Dead-internal reachability analysis | Rejected insufficient signal for external linkage | It removes unreachable discardable definitions and uses compatibility scans; it does not publish durable no-external-caller metadata. |
| Prepared value homes / pointer-base-plus-offset facts | Rejected as authority source | Useful after formal provenance is authorized, but cannot prove complete incoming values for a formal. |

Audited surface references:

- HIR `Linkage`: `src/frontend/hir/hir_ir.hpp:477`; function linkage is
  populated in `src/frontend/hir/hir_functions.cpp:598`.
- HIR-to-LIR rendering/lowering: definitions render `internal` only for static
  linkage in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:902`; definitions set
  `LirFunction::is_internal = fn.linkage.is_static` in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1606`.
- LIR function fields: `src/codegen/lir/ir.hpp:540` carries `is_internal`,
  `can_elide_if_unreferenced`, `is_declaration`, and identity fields.
- BIR function metadata: `src/backend/bir/bir.hpp:2731` has no internal,
  private, closed-world, visibility, or no-external-caller field.
- BIR call/source metadata: `src/backend/bir/bir.hpp:1196` and `:1225` carry
  argument source facts and direct callee identity for observed calls only.
- Current idea 442 formal provenance collector:
  `src/backend/bir/lir_to_bir/module.cpp:316` admits same-module callees only
  through internal name/link-id sets built at
  `src/backend/bir/lir_to_bir/module.cpp:1567`.
- Prepared module and call plans: `src/backend/prealloc/module.hpp:39` wraps
  BIR plus prepared plans but has no closed-world authority container;
  `src/backend/prealloc/calls.hpp:824` and `:874` classify observed calls, not
  complete callee caller sets.

Missing metadata:

- No durable BIR/prepared function-level fact currently states
  closed-world/private/no-external-caller authority for externally visible
  definitions.
- No complete callgraph fact rejects function-address escape, unknown indirect
  callers, declarations, or external callers for a non-internal definition.
- No prepared authority field is available for idea 442 to consume beyond
  `LirFunction::is_internal`.

Artifacts:

- `build/agent_state/443_step1_closed_world_authority_audit/audit.md`

## Suggested Next

Execute Step 2: Define The Authority Contract.

Recommended Step 2 packet: write the producer contract for formal pointer
authority before implementation. The contract should preserve
`LirFunction::is_internal` as accepted baseline authority, define any new
closed-world/private/no-external-caller fact as false by default for
external-linkage definitions, and explicitly reject observed same-module
calls, prepared same-module wrappers, `LinkNameId`, non-declaration status,
visibility, and discardability as standalone authority.

Suggested owned files:

- `todo.md`
- `build/agent_state/443_step2_closed_world_authority_contract/*`

Proof:

```sh
git diff --check
```

## Watchouts

- Do not publish formal pointer provenance for external-linkage callees from
  observed same-module direct callsites alone.
- Do not treat hidden/protected visibility, same-module call wrappers,
  `LinkNameId`, or `can_elide_if_unreferenced` as no-external-caller authority
  without a separately defined and tested contract.
- Do not mark `930930-1` done while `%lv.param.reg1` and `%mr_TR - 8` rows
  remain `layout_authority=unknown` and
  `range_verdict=unknown_compatible`.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Keep pointer-delta propagation behind proven base formal pointer authority.
- Do not accept or commit the rejected `test_baseline.new.log` full-suite
  baseline refresh with failing `string_authority_guard`.

## Proof

Step 1 classification-only proof:

```sh
git diff --check
```
