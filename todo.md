Status: Active
Source Idea Path: ideas/open/433_rv64_global_select_pointer_memory_residuals.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-probed and classified residuals after the selected RV64
global object-data implementation.

Fresh representative status:

- `20011019-1`: prepared probe succeeded and RV64 object probe now succeeds.
  This row moved past selected global object-data and no longer reports
  `unsupported_global_data: prepared selected object-data contract
  status=unsupported_but_coherent`.
- `930930-1`: still fails with `unsupported_instruction_fragment`; evidence
  remains pointer-value memory, global memory publication with
  `source_producer=unknown`, local store publication, and select
  materialization.
- `20000622-1`: still fails with `unsupported_instruction_fragment`; evidence
  remains select/local-publication residuals plus already-covered pointer-cast
  forms.
- `20010329-1`: still fails with `unsupported_terminator_fragment`; remaining
  owner is terminator/select publication, not pointer/address movement or
  global object-data.
- `20041112-1`: still fails with `unsupported_terminator_fragment`; evidence
  remains direct-global return/terminator publication plus global load/store
  and select-chain facts.

Current residual buckets:

- Selected global object-data: completed by Step 3; representative
  `20011019-1` succeeds.
- Global memory publication: remains in `930930-1` and `20041112-1`, but needs
  explicit source/publication authority before more RV64 lowering.
- Direct-global select/return: remains in `20041112-1`; do not infer return
  authority from raw `bir.ret ptr @global`.
- Terminator/select publication: remains in `20010329-1`, `20041112-1`, and
  select residuals in `20000622-1`/`930930-1`.
- Pointer-value memory: remains in `930930-1`; producer/prepared layout and
  provenance authority are still missing.
- Local/store-source producer gaps: remain in `930930-1`, `20000622-1`, and
  `20041112-1`.

Artifacts:

- `build/agent_state/433_step4_residual_disposition/probe_summary.tsv`
- `build/agent_state/433_step4_residual_disposition/evidence_snippets.txt`
- `build/agent_state/433_step4_residual_disposition/classification.md`

## Suggested Next

Plan-owner close-readiness review for this source idea.

Recommended disposition:

- Treat idea 433 as close-ready for the bounded residual triage plus first
  coherent object-data packet.
- Route remaining failures to separate follow-up ideas instead of selecting
  another packet inside this runbook:
  producer/prepared pointer-value memory authority, store-source/global-memory
  publication, direct-global return/select-chain authority, and
  terminator/select publication.

Suggested close-review proof/reference command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not reopen bounded `inttoptr`/`ptrtoint` pointer-cast movement from idea
  429.
- Do not infer global object identity, symbol addressability, relocation base,
  memory layout, select-chain roots, or return pointer authority in RV64 from
  raw BIR or testcase shape.
- Keep aggregate ABI, F128, scalar FP, div/rem, and unrelated call-publication
  work out of scope.
- If evidence shows a producer/prepared gap, route it to a separate source idea
  instead of bypassing it in RV64.
- Do not infer global object identity, byte layout, zero-fill, relocation base,
  or symbol addressability from raw BIR, testcase filenames, function names, or
  object label numbers.
- Do not claim all residual rows pass. Only `20011019-1` is newly green in the
  representative set; the other rows remain separate residual families.

## Proof

Step 4 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
