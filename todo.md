Status: Active
Source Idea Path: ideas/open/435_rv64_coherent_aggregate_sret_call_storage_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 3: re-probed caller-side aggregate `sret` memory-return and
classified the implementation boundary as no-op for this packet.

Boundary summary:

- Fresh probes for `20000917-1.c` and `20020206-1.c` confirm the in-scope
  same-module aggregate `sret` call-storage facts remain complete:
  `memory_return`, `memory_encoding=frame_slot`, `sret_arg_index=0`,
  `memory_size=12`, `memory_align=4`, aggregate-address arg0, and local
  frame-slot address materialization.
- Existing RV64 object lowering already has a prepared-fact-driven
  same-module `sret` memory-return path, and Step 2 focused coverage now proves
  the representative 12-byte / align-4 contract plus fail-closed shapes.
- The representative post-call field-copy facts are explicit prepared
  frame-slot accesses at offsets `0`, `4`, and `8`; this packet did not
  rederive aggregate layout or offsets in RV64.
- `--codegen obj` still reports the generic RV64 object-route unsupported
  diagnostic, while `--codegen asm` resolves the first non-sret residual to
  unsupported runtime external `abort` (`main` in `20000917-1.c`, `baz` in
  `20020206-1.c`).
- No implementation files changed for Step 3.
- Step 3 artifacts: `build/agent_state/435_step3_sret_memory_return/`.

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

Recommended scope: classify the remaining representative blockers as
out-of-scope runtime external `abort`/`exit`, select/local-publication, or
post-call-copy residuals, then decide whether this source idea is ready for
lifecycle close review or needs a new non-sret follow-up idea.

Implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not include `src/pr88904.c`; it belongs to idea 436 until producer facts
  are reconciled.
- Do not include scalar metadata defect rows `src/20010224-1.c` or
  `src/20020506-1.c` as aggregate lowering proof.
- Do not infer aggregate layout, size, alignment, field offsets, return slots,
  or storage homes in RV64.
- Do not route unsupported runtime external `abort`/`exit` calls into this
  aggregate sret plan; they are the first non-sret residuals after the coherent
  facts in the representatives.
- The focused object fixture validates the call-storage contract, and Step 3
  found no remaining sret call-storage implementation gap.
- Treat representative field-copy facts as prepared-addressing/post-call-copy
  evidence; do not rederive aggregate layout in RV64.
- Keep select, local-publication, pointer, inline-asm, F128, and expectation
  work out of this plan unless Step 4 routes residuals separately.

## Proof

Delegated proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
