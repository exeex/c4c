Status: Active
Source Idea Path: ideas/open/443_closed_world_formal_pointer_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Authority Contract

# Current Packet

## Just Finished

Completed Step 2: defined the producer authority contract for closed-world
formal pointer provenance and selected the first executable packet.

Contract:

- Formal pointer provenance publication may consume same-module call argument
  sources only when a producer-owned function authority fact proves the callee
  formal's incoming values are closed over the observed same-module call edges.
- Required facts are stable callee identity, definition status,
  no-external-caller/closed-world authority, observed callsite completeness,
  coherent computed-address argument sources for each accepted callsite, and a
  clear consumer boundary for idea 442.
- `LirFunction::is_internal` from static linkage remains the accepted baseline
  authority and must continue to work exactly as committed in idea 442.
- All non-internal definitions are false by default until a separate explicit
  producer fact proves closed-world/no-external-caller authority.
- External-linkage definitions such as `930930-1::f` remain fail-closed even
  when every currently observed same-module call passes coherent
  computed-address arguments.

Rejected adjacent shapes:

| Signal | Contract verdict |
| --- | --- |
| Observed same-module direct calls | Not authority without a no-external-caller fact. |
| `PreparedCallWrapperKind::SameModule` | Observed edge classification only. |
| `CallArgumentSourceRelationship::ComputedAddress` | One argument source only, not formal incoming-value completeness. |
| `LinkNameId` / `FunctionSymbolSet` | Identity only, not closed-world authority. |
| `bir::Function::is_declaration == false` | Definition status only; definitions can still be externally callable. |
| Hidden/protected/default visibility | Rejected until a separate tested contract proves its semantics for this compilation model. |
| `LirFunction::can_elide_if_unreferenced` / reachability | Emission/discardability, not formal provenance authority. |
| Prepared value homes / pointer-base-plus-offset facts | Value facts after authority, not authority for formal incoming values. |

Artifacts:

- `build/agent_state/443_step2_closed_world_authority_contract/contract.md`

## Suggested Next

Execute Step 3: Implement Or Route Focused Producer Coverage.

Recommended Step 3 packet: add the smallest durable producer/prepared authority
surface and focused coverage without extending external-linkage provenance.

Implementation target:

- Add a function-level formal pointer authority fact, for example
  `InternalOnly`/`NoExternalCaller`/`Unknown` or equivalent naming.
- Populate it from `LirFunction::is_internal` only.
- Preserve idea 442 behavior by changing same-module formal provenance
  collection to consume this authority surface instead of a private ad hoc
  internal-name/internal-link-id set.
- Add focused tests proving static/internal functions receive accepted
  authority and external-linkage functions remain absent/unknown/fail-closed.
- Do not publish `930930-1::f` provenance or implement pointer-delta
  propagation in this packet.

Suggested owned files:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_printer/functions.cpp` or adjacent prepared
  printer file if the new fact is dumped
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp` or a focused
  prepared contract test file
- `todo.md`
- `test_after.log`
- `build/agent_state/443_step3_closed_world_authority_surface/*`

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not publish formal pointer provenance for external-linkage callees from
  observed same-module direct callsites alone.
- Do not treat hidden/protected visibility, same-module call wrappers,
  `LinkNameId`, or `can_elide_if_unreferenced` as no-external-caller authority
  without a separately defined and tested contract.
- In Step 3, the new authority surface should initially be a refactor/contract
  carrier for the already-accepted internal/static case, not a claim that
  external-linkage formal provenance is now solved.
- If adding the authority surface proves too broad, stop and ask plan-owner to
  split the metadata-carrier design instead of routing through observed
  callsites.
- Do not mark `930930-1` done while `%lv.param.reg1` and `%mr_TR - 8` rows
  remain `layout_authority=unknown` and
  `range_verdict=unknown_compatible`.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Keep pointer-delta propagation behind proven base formal pointer authority.
- Do not accept or commit the rejected `test_baseline.new.log` full-suite
  baseline refresh with failing `string_authority_guard`.

## Proof

Step 2 contract-only proof:

```sh
git diff --check
```
