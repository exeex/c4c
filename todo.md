Status: Active
Source Idea Path: ideas/open/429_rv64_pointer_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-Probe Representatives And Decide Close Readiness

# Current Packet

## Just Finished

Completed Step 3, Lower Coherent Address Materialization.

No new implementation route was added for this step. The representative
evidence does not contain an explicit prepared address-materialization fact
that would authorize a new RV64 target lowering path.

Fresh Step 3 probes are under
`build/agent_state/429_step3_address_materialization/`:
`probe_summary.tsv`, `evidence_snippets.txt`, `classification.md`, and per-row
prepared/object logs.

Existing RV64 object routes already consume explicit prepared facts for:

- frame-slot address materialization via
  `PreparedAddressMaterializationKind::FrameSlot`;
- PC-relative string/direct-global address materialization via prepared symbol
  materialization facts.

Existing backend tests already cover these supported surfaces:
`builds_prepared_frame_slot_address_local_store_object`,
`emits_prepared_string_address_relocations_to_object_symbol`, and
`emits_prepared_global_address_relocations_to_object_symbol`.

Representative Step 3 classification:

- `src/930930-1.c`: pointer-base-plus-offset homes and a global `mem` store
  access exist, but no `PreparedAddressMaterialization` record authorizes a
  standalone address value route. Keep as pointer-value memory/global-store
  residual.
- `src/20000622-1.c`: Step 2 pointer-cast shape exists; no address
  materialization facts are present.
- `src/20010329-1.c`: pointer casts remain visible, but fresh failure is an
  unsupported terminator/select-publication shape, not address materialization.
- `src/20011019-1.c`: global pointer load/store and direct-global select chain
  evidence remains; fresh failure is unsupported global object data, not a
  missing address-materialization consumer.
- `src/20041112-1.c`: raw `bir.ret ptr @global`, global loads/stores, and
  direct-global select chains appear without a prepared address-materialization
  record for the return. Do not infer target authority from raw `@global`.

## Suggested Next

Execute Step 4: re-probe representatives and decide close readiness for the
source idea. Treat remaining global object-data/global-memory/direct-global
return and select-chain residuals as follow-up candidates unless fresh evidence
shows explicit prepared address-materialization facts.

## Watchouts

- Do not infer object identity, provenance, relocation base, or addressability
  from integer shape, source filenames, raw BIR, or representative row text.
- Existing frame-slot/string/global address materialization tests prove the
  prepared-fact routes already present; do not duplicate them with raw-BIR
  inference.
- Treat direct `bir.ret ptr @global`, direct-global select chains, global
  object-data failures, and global load/store addressing as producer or
  global-memory follow-up work unless prepared facts are added.
- Pointer-base-plus-offset homes authorize memory base-plus-offset access when
  the access facts are coherent; they are not standalone address
  materialization authority for this packet.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 3 classification/no-new-implementation proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
