Status: Active
Source Idea Path: ideas/open/435_rv64_coherent_aggregate_sret_call_storage_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Probe Coherent SRet Residual Shape

# Current Packet

## Just Finished

Activated `ideas/open/435_rv64_coherent_aggregate_sret_call_storage_lowering.md`
as the next active plan. Selection rationale: the scalar call ABI metadata
producer defect split from 431 has closed in 434, and 435 explicitly excludes
`pr88904`; the remaining coherent `20000917-1`/`20020206-1` aggregate `sret`
rows are now the next safe follow-up.

## Suggested Next

Execute Step 1: probe the coherent aggregate `sret` representatives and choose
the first bounded implementation/test packet from the exact RV64 unsupported
fragment and prepared memory-return facts.

## Watchouts

- Do not include `src/pr88904.c`; it belongs to idea 436 until producer facts
  are reconciled.
- Do not include scalar metadata defect rows `src/20010224-1.c` or
  `src/20020506-1.c` as aggregate lowering proof.
- Do not infer aggregate layout, size, alignment, field offsets, return slots,
  or storage homes in RV64.
- Keep select, local-publication, pointer, inline-asm, F128, and expectation
  work out of this plan unless Step 4 routes residuals separately.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
