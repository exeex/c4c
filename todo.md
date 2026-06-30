Status: Active
Source Idea Path: ideas/open/439_store_source_global_memory_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Publication Authority Contract

# Current Packet

## Just Finished

Completed Step 2: defined the publication authority contract for
store-source/global-memory rows and selected the first executable packet.

Required global-memory facts:

- prepared global symbol identity;
- access kind: load, store, or address-only publication;
- byte offset, width, and alignment;
- explicit layout/addressability authority for the global object and range;
- range verdict plus dynamic-array/flexible-object verdict when applicable;
- a statement of whether the row is allowed as data load/store, address
  materialization, or direct-global return/select-chain publication.

Required store-source facts:

- exact store-source identity for a value or immediate;
- producer kind, producer block, and producer instruction when applicable;
- current value home for non-immediate stores, including bank/register or
  stack-slot home and width;
- explicit immediate-source encoding for immediate stores, including value and
  width;
- publication intent distinguishing global store, local store, and
  pointer-value memory store.

Classification:

| class | records | disposition |
| --- | --- | --- |
| Accepted target-consumable records | none from Step 1 evidence | No RV64 lowering packet is selected yet. |
| Producer/prepared gaps | `930930-1` `@mem+792` store; `20041112-1` binary stores to `@global+0`; `20041112-1` immediate store to `@global+0` | Need producer/prepared authority before target lowering. |
| Fail-closed unknowns | `source_producer=unknown`, `source=<none>` without immediate encoding, `layout_authority=unknown`, missing value home | Keep rejected. |
| Rejected adjacent shapes | pointer-value memory, local frame-slot store publication, direct-global return/select-chain, select/terminator residuals | Out of scope for this packet. |

Artifact:

- `build/agent_state/439_step2_publication_authority_contract/contract.md`

## Suggested Next

Execute Step 3: Implement Or Route First Publication Packet.

Selected packet: producer/prepared contract coverage for global-memory
publication authority. Add focused positive/negative coverage for explicit
global layout authority, explicit store-source producer/value home, explicit
immediate-source encoding, and fail-closed `source_producer=unknown` plus
`layout_authority=unknown`. Only change producer code if focused coverage
exposes a local metadata gap. Do not edit RV64 target lowering until accepted
prepared facts exist.

## Watchouts

- Do not infer store-source identity, global object identity, offset meaning,
  layout, addressability, or value home from raw BIR, testcase names, object
  labels, symbol spelling, or dump shape.
- Keep `source_producer=unknown` fail-closed until producer facts are explicit.
- Do not fold pointer-value memory, direct-global return/select-chain, or
  terminator/select publication work into this plan.
- Do not return to parked idea 442 for external-linkage formal provenance
  without a new source of closed-world/no-external-caller authority.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 2 contract-only check:

```sh
git diff --check
```
