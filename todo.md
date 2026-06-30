Status: Active
Source Idea Path: ideas/open/439_store_source_global_memory_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-checked representative residuals against the Step 3
global-memory publication authority predicates and recorded close readiness.

Residual table:

| row | residual | authority result | disposition |
| --- | --- | --- | --- |
| `930930-1` | `main.entry.0` stores `%t1` to `@mem+792` | fail-closed | `source_producer=unknown` and `layout_authority=unknown`; producer gap, not RV64 target work. |
| `20000622-1` | `baz.entry.0` local frame-slot store-source | out of scope | no global memory row; local publication/select residual. |
| `20041112-1` | `foo.block_1.2` stores binary `%t4` to `@global+0` | fail-closed | binary source has home, but global access still has `layout_authority=unknown`. |
| `20041112-1` | `bar.block_5.2` stores binary `%t10` to `@global+0` | fail-closed | binary source has home, but global access still has `layout_authority=unknown`. |
| `20041112-1` | `main.entry.0` stores immediate `1` to `@global+0` | fail-closed | lacks explicit immediate-source encoding and global layout authority. |
| `20041112-1` | `bir.ret ptr @global` and direct-global select-chain facts | out of scope | separate direct-global return/select-chain authority family. |
| `930930-1` | pointer-value memory through `%t27` | out of scope | separate pointer-value memory authority family. |

Close-readiness decision:

- Idea 439 is close-ready as producer/prepared contract and coverage.
- Step 3 added durable authority predicates and focused fail-closed coverage.
- Representative rows still fail because producer facts are absent, not because
  RV64 should infer them.
- Remaining immediate global-store source encoding and global layout authority
  are distinct producer repairs and should be split if selected.

Artifacts:

- `build/agent_state/439_step4_residual_disposition/classification.md`
- `build/agent_state/439_step4_residual_disposition/evidence_snippets.txt`

## Suggested Next

Plan-owner close review for idea 439.

Recommendation: close this plan as contract/coverage complete. If the next
residual is selected, split a new producer idea for immediate global-store
source encoding and/or global layout authority publication. Do not continue
inside 439 by editing RV64 target lowering.

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

Step 4 residual-disposition proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
