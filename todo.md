Status: Active
Source Idea Path: ideas/open/429_rv64_pointer_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-Probe Representatives And Decide Close Readiness

# Current Packet

## Just Finished

Completed Step 4, Re-Probe Representatives And Decide Close Readiness.

Fresh close-readiness probes are under
`build/agent_state/429_step4_close_readiness/`: `probe_summary.tsv`,
`evidence_snippets.txt`, `classification.md`, and per-row prepared/object logs.

Acceptance coverage from this plan:

- Step 2 added focused RV64 object lowering for coherent prepared scalar
  pointer-cast movement: `IntToPtr` from `i32`/`i64` to `ptr` and `PtrToInt`
  from `ptr` to `i64`, restricted to authoritative GPR-compatible prepared
  homes plus literal/rematerializable integer sources.
- Step 3 found no additional representative-owned target address-materialization
  packet. Existing RV64 routes already consume explicit prepared frame-slot,
  string, and direct-global address materialization facts.

Fresh representative results:

- `src/930930-1.c`: still rejects with `unsupported_instruction_fragment`.
  Evidence has stack-result pointer casts, pointer-base-plus-offset homes,
  computed `@mem` call-arg sources, select materialization, and a global `mem`
  store. Classify as pointer-value memory/global-store/select residual.
- `src/20000622-1.c`: still rejects with `unsupported_instruction_fragment`.
  The in-scope `inttoptr i32 12 to ptr` shape is covered by Step 2 focused
  tests; the full row has additional call/select/local-publication residuals.
- `src/20010329-1.c`: still rejects with `unsupported_terminator_fragment`.
  The in-scope immediate/register `inttoptr` shapes are covered by Step 2
  focused tests; remaining failure is terminator/select-publication behavior.
- `src/20011019-1.c`: still rejects with `unsupported_global_data:
  prepared selected object-data contract status=unsupported_but_coherent ...`.
  Classify as global object-data plus global pointer load/store/direct-global
  select-chain follow-up.
- `src/20041112-1.c`: still rejects with `unsupported_terminator_fragment`.
  Evidence includes raw `bir.ret ptr @global`, global loads/stores,
  direct-global select chains, and global access facts without a prepared
  address-materialization record for the return. Classify as producer/global
  addressing or terminator follow-up.

Close-readiness recommendation: this source idea appears ready for lifecycle
close review as a bounded target-side pointer/address materialization effort,
but not as "all representative rows pass." Remaining rows require separate
follow-up disposition for global object-data/global-memory, direct-global
return/select-chain, terminator/select-publication, and pointer-value memory
residual classes.

## Suggested Next

Ask the plan owner to close or retire this runbook with explicit follow-up
disposition for the remaining residual classes, or split the next source idea
for global object-data/global-memory/direct-global return and select-chain
work if the supervisor wants to continue from these representatives.

## Watchouts

- Do not infer object identity, provenance, relocation base, or addressability
  from integer shape, source filenames, raw BIR, or representative row text.
- Treat direct `bir.ret ptr @global`, direct-global select chains, global
  object-data failures, and global load/store addressing as producer or
  global-memory follow-up work unless prepared facts are added.
- Pointer-base-plus-offset homes authorize memory base-plus-offset access when
  the access facts are coherent; they are not standalone address
  materialization authority for this packet.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 4 close-readiness proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
