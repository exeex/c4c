Status: Active
Source Idea Path: ideas/open/439_store_source_global_memory_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Store-Source And Global-Memory Evidence

# Current Packet

## Just Finished

Completed Step 1: audited store-source and global-memory residual evidence for
`930930-1`, `20000622-1`, and `20041112-1`.

Bucket table:

| bucket | rows | representative evidence | decision |
| --- | --- | --- | --- |
| Accepted target-consumable candidates | none | No inspected global-symbol row has both explicit global layout/addressability authority and a complete acceptable store-source/value-home contract. | No implementation packet selected from Step 1. |
| Producer/prepared gaps | `930930-1` `main.entry.0`; `20041112-1` `foo.block_1.2`, `bar.block_5.2`, `main.entry.0`; local store-source rows in all three representatives | `930930-1` global store has `source_producer=unknown`; `20041112-1` known binary stores have register homes but `layout_authority=unknown`; immediate store has `source=<none>` and `source_producer=unknown`. | Needs a publication-authority contract before producer or target code is selected. |
| Fail-closed unknowns | `930930-1` global store and pointer-value memory; `20000622-1` local store-source; `20041112-1` immediate global store | Unknown producer/layout facts are not target-consumable. | Keep fail-closed. |
| Unrelated/non-goals | `930930-1` pointer-value memory; `20000622-1` select materialization; `20041112-1` direct-global return/select-chain and terminator failure | These map to pointer-value memory, select/terminator publication, or direct-global return authority. | Do not fold into this plan packet. |

Representative row notes:

- `930930-1`: `bir.store_global @mem, offset 792, i64 %t1` has stored value
  `%t1` with a register home in `main`, but the store-source publication is
  `source_producer=unknown`, and the global access has
  `layout_authority=unknown`.
- `20000622-1`: no global memory row; only a local frame-slot store-source
  row with `source_producer=unknown` plus select-materialization residuals.
- `20041112-1`: `foo` and `bar` global stores to `@global` at offset 0 have
  binary store-source producers and register value homes for `%t4`/`%t10`, but
  the global accesses still report `layout_authority=unknown`; `main.entry.0`
  stores immediate `1` but its store-source row is `source=<none>` with
  `source_producer=unknown`.

Artifacts:

- `build/agent_state/439_step1_store_source_global_memory_audit/audit.md`
- `build/agent_state/439_step1_store_source_global_memory_audit/rows.tsv`

## Suggested Next

Execute Step 2: Define Publication Authority Contract.

Recommended packet: contract-only. Define the minimum prepared facts required
for global symbol identity, offset, width, alignment, layout/addressability
authority, store-source identity, producer kind, value home, and immediate
source encoding. Keep `source_producer=unknown`, `layout_authority=unknown`,
pointer-value memory, direct-global return, and select/terminator residuals
fail-closed.

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

Step 1 classification-only check:

```sh
git diff --check
```
