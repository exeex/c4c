Status: Active
Source Idea Path: ideas/open/444_no_external_caller_formal_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Producer Contract

# Current Packet

## Just Finished

Completed Step 2: defined the `NoExternalCaller` producer contract and
classified the implementation route as blocked.

Contract summary:

- `FormalPointerAuthorityKind::NoExternalCaller` may be produced for a
  non-internal definition only from a durable function-level fact proving that
  same-module call argument sources are the complete authority for that
  function's formal pointer values.
- Required facts include semantic function identity, definition status,
  explicit closed-world/no-external-caller authority, complete caller-set
  coverage, function-address escape coverage, indirect-call target exclusion,
  and any visibility/linker authority backed by focused positive and negative
  tests.
- False by default: all non-internal definitions remain
  `FormalPointerAuthorityKind::Unknown` unless the complete contract is
  satisfied. `930930-1::f` remains `Unknown` and fail-closed.
- Rejected adjacent shapes include observed same-module direct calls,
  same-module definition/non-declaration status, `LinkNameId`, extern-decl
  absence, prepared call-plan metadata, hidden/protected/default visibility
  alone, `can_elide_if_unreferenced`, missing observed address initializers,
  and missing observed indirect calls.
- Consumer boundary: ideas 442 and 443 may consume only
  `bir::formal_pointer_authority_allows_same_module_call_sources()` and must
  not re-derive authority from raw BIR, prepared dumps, callsite counts, symbol
  names, visibility text, or representative row shape.

Implementation decision:

- No bounded Step 3 implementation packet is available in the current pipeline.
- Step 1 found no durable closed-world/no-external-caller fact, complete
  caller-set guarantee, function-address escape summary, indirect-call target
  exclusion, or tested visibility/linker contract for non-internal definitions.
- `FormalPointerAuthorityKind::NoExternalCaller` must remain unpopulated until
  a separate producer initiative supplies one of those facts.

Artifact:

- `build/agent_state/444_step2_no_external_caller_contract/contract.md`

## Suggested Next

Plan-owner disposition.

Recommended route: close this plan as contract/blocker documentation, or split
a new metadata producer initiative for a real no-external-caller/closed-world
source. Do not route external-linkage formal provenance back to idea 442 and do
not select a Step 3 code packet until such a producer source exists.

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

Step 2 contract-only check:

```sh
git diff --check
```
