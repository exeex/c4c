Status: Active
Source Idea Path: ideas/open/447_immediate_global_store_source_encoding.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Immediate Global-Store Evidence

# Current Packet

## Just Finished

Completed Step 1: audited immediate-valued global-store evidence for idea 447
and recorded classifications under
`build/agent_state/447_step1_immediate_global_store_audit/`.

Immediate-valued global-store bucket table:

| row | function/block/inst | destination | offset | store width | current layout authority | immediate value | current source encoding | first missing producer fact | bucket |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `20041112-1` | `main/entry/0` | `@global` | `0` | `4` | `scalar_layout` | `1` | `source=<none>; source_producer=unknown; intent=store_global_publication` | explicit immediate-source producer/encoding for the `i32 1` store value | coherent destination/layout candidate blocked on immediate source encoding |
| `20041112-1` | `foo/block_1/2` | `@global` | `0` | `4` | `scalar_layout` | n/a | `source=%t4; source_producer=binary` | none for this plan; source is already a non-immediate value-producing binary | out of scope for immediate-source encoding |
| `20041112-1` | `bar/block_5/2` | `@global` | `0` | `4` | `scalar_layout` | n/a | `source=%t10; source_producer=binary` | none for this plan; source is already a non-immediate value-producing binary | out of scope for immediate-source encoding |
| `930930-1` | `main/entry/0` | `@mem` | `792` | `8` | `byte_storage_aggregate` after idea 448 | n/a | `source=%t1; source_producer=unknown` | not an immediate literal source; remaining source gap belongs outside this immediate-source packet | out of scope for immediate-source encoding |
| `20000622-1` | `baz/entry/0` | n/a | n/a | `8` | `unknown` | n/a | `source=%t0; intent=store_local_publication` | no global-store row | out of scope local/frame-slot store |

Fresh `20041112-1` prepared probe confirms the inherited 439 blocker has
narrowed: `main.entry.0` now has destination/global layout facts
(`layout_authority=scalar_layout`, `range_verdict=proven_in_bounds`) but still
publishes `source=<none>` and `source_producer=unknown` for `bir.store_global
@global, i32 1`.

## Suggested Next

Execute Step 2: define the immediate source encoding contract. The first
bounded implementation packet after that contract should publish a
producer-owned immediate source record for immediate-valued `store_global` rows
only when destination global publication facts are already coherent. Required
contract facts should include immediate value/type/width or exact bit-pattern,
destination global identity, offset, width, alignment, layout/range authority,
and `store_global_publication` intent.

## Watchouts

- Keep this plan limited to immediate global-store source encoding.
- Do not reopen scalar or integer-array global layout authority; those are
  closed by ideas 446 and 448.
- Do not infer immediate values or source publication in RV64 from raw BIR
  store shape, testcase names, block labels, symbol names, or one dump layout.
- Keep `source=<none>` and `source_producer=unknown` fail-closed until
  producer facts are explicit.
- Keep non-immediate global stores (`source=%t4`, `%t10`, `%t1`), local stores,
  pointer-value memory, direct-global return/select-chain rows, and target
  lowering outside Step 2.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 1 classification-only validation:

```sh
git diff --check
```
