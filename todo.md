Status: Active
Source Idea Path: ideas/open/448_array_aggregate_global_layout_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Non-Scalar Global Layout Evidence

# Current Packet

## Just Finished

Completed Step 1: audited inherited non-scalar prepared global-symbol memory
evidence from the 446 Step 4 residual artifacts.

Representative row table:

| row | evidence | current authority | first missing producer fact |
| --- | --- | --- | --- |
| `930930-1 @mem+792` | Source declares `ptr_t mem[100]`; prepared BIR stores `%t1` to `@mem+792`, width 8, align 8; call-argument source comments use computed addresses from `@mem` at deltas 800, 48, 64, 792, and 792. | `layout_authority=unknown`, `range_verdict=proven_in_bounds`, `dynamic_array_verdict=unknown`; object extent/completeness is not printed. | Explicit non-scalar layout authority kind plus durable producer facts for semantic `@mem` shape, complete extent, element stride/count or aggregate layout, offset, width, alignment, and range. |

Bucket table:

| bucket | rows | disposition |
| --- | --- | --- |
| Coherent non-scalar authority candidate | `930930-1` `main.entry.0` store to `@mem+792` | Only inherited non-scalar global-symbol memory row. It has semantic global symbol, offset, width, alignment, and a proven in-bounds range, but remains fail-closed because layout authority is unknown. |
| Missing/incoherent authority | `930930-1 @mem+792` | Prepared evidence does not publish a non-scalar layout-authority kind or visible object shape/extent facts for consumers, even though range proof is in bounds. |
| Out-of-scope rows | `20041112-1 @global+0` scalar rows; `20041112-1` direct-global address/return row; `20000622-1` frame-slot rows | Scalar global layout authority was completed by idea 446. Direct-global return/select-chain, local/frame-slot rows, pointer-value memory, and immediate store-source encoding are separate owners. |
| Blockers | none for Step 2 contract | Evidence is sufficient to define the contract before implementation. |

Artifacts:

- `build/agent_state/448_step1_non_scalar_global_layout_audit/audit.md`
- `build/agent_state/448_step1_non_scalar_global_layout_audit/rows.tsv`

## Suggested Next

Execute Step 2: Define Array/Aggregate Layout Authority Contract.

Recommended contract focus: decide whether the first implementation packet is
integer-array global layout authority for semantic globals with producer-owned
element stride, element count, complete storage size, and linear addressability,
or whether a durable producer metadata gap blocks implementation. The likely
first implementation target after the contract is a narrow integer-array
carrier in the BIR/prepared producer path, similar in shape to scalar global
authority but explicitly non-scalar and not keyed to `@mem`, offset `792`, or
RV64 target behavior.

## Watchouts

- Keep this plan limited to array/aggregate global layout authority.
- Do not infer non-scalar layout authority in RV64 from raw BIR, symbol names,
  object labels, representative filenames, offsets, or dump shape.
- Do not claim scalar `layout_authority=scalar_layout` rows as progress for
  this non-scalar plan.
- Keep immediate global-store source encoding with
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Keep direct-global return/select-chain, pointer-value memory,
  terminator/select publication, and local/frame-slot residuals outside this
  idea.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 1 classification-only check:

```sh
git diff --check
```
