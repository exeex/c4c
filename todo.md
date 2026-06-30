Status: Active
Source Idea Path: ideas/open/446_global_memory_layout_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Global Layout Authority Evidence

# Current Packet

## Just Finished

Completed Step 1: audited prepared global-symbol memory rows from the 439 Step
4 residual evidence.

Representative rows:

| row | evidence | current authority | first missing producer fact |
| --- | --- | --- | --- |
| `930930-1` | `main.entry.0` stores `%t1` to `@mem+792`, width 8, align 8, range `proven_in_bounds` | `layout_authority=unknown`; object extent/completeness not printed | global layout authority plus object extent/completeness for `@mem+792` |
| `20000622-1` | no global-symbol memory row | n/a | out of scope for this layout-authority plan |
| `20041112-1` | loads/stores/address-only rows for `@global+0`, width 4, align 4, range `proven_in_bounds` | all `layout_authority=unknown`; object extent/completeness not printed | global layout authority plus object extent/completeness for `@global+0` |

Bucket table:

| bucket | rows | disposition |
| --- | --- | --- |
| Coherent layout-authority candidates | `20041112-1` global load/store rows for `@global+0` | Symbol, offset, width, alignment, and range are present; plausible positive contract examples once extent/completeness and layout authority are explicit. |
| Missing/incoherent authority | `930930-1` `@mem+792`; all `20041112-1` global-symbol rows | `layout_authority=unknown`; object extent/completeness unavailable in prepared dumps, so idea 439 predicates fail closed. |
| Out-of-scope rows | `20000622-1`; `20041112-1` direct-global return/address-only row; pointer-value and frame-slot rows | Not Step 1 layout-authority implementation targets; immediate store-source encoding belongs to idea 447. |
| Blockers | none for Step 2 contract; code packet not selected | Need a contract and producer source selection before implementation. |

Artifacts:

- `build/agent_state/446_step1_global_layout_authority_audit/audit.md`
- `build/agent_state/446_step1_global_layout_authority_audit/rows.tsv`

## Suggested Next

Execute Step 2: Define Global Layout Authority Contract.

Recommended packet: contract-only. Define required facts for global symbol
identity, global object definition/status, addressability, complete object
extent, offset, width, alignment, range proof, and accepted non-unknown layout
authority. Keep `layout_authority=unknown`, missing object extent, direct-global
return/address-only rows, pointer-value memory, and immediate store-source
encoding fail-closed.

## Watchouts

- Keep this plan limited to global layout authority publication.
- Do not fold immediate store-source encoding into this plan; that belongs to
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Do not infer global layout, object extent, addressability, offset meaning,
  or symbol identity in RV64 from raw BIR, symbol spelling, object labels,
  representative filenames, or dump shape.
- Keep `layout_authority=unknown` fail-closed until producer facts are
  explicit.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 1 classification-only check:

```sh
git diff --check
```
