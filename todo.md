Status: Active
Source Idea Path: ideas/open/640_mixed_local_global_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Mixed Publication Residuals

# Current Packet

## Just Finished

Step 1 refreshed the five mixed-publication residual rows with the supervisor
allowlist and fresh prepared dumps under `build/agent_state/640_step1_*`.

All five rows still stop in the RV64 object route with
`unsupported_local_memory_access: RV64 object route requires prepared
frame-slot or pointer-value base-plus-offset local memory`, so none prove a
remaining direct `addr @symbol` local-memory consumer gap from closed idea 631.

Row evidence:

- `src/pr57861.c`: first owner is the RV64 local-memory consumer. First missing
  authority fact is a scalar local publication/consumer rule for `%lv.k`
  (`bir.store_local %lv.k` from a direct-global-derived chain, then
  `bir.load_local %lv.k`); the later `short *l = &f` pointer-local path remains
  fail-closed as pointer/global publication rather than direct global-symbol
  local memory.
- `src/pr58431.c`: first owner is the RV64 local-memory consumer. First missing
  authority fact is scalar frame-slot local publication/consumption for
  globals/casts copied through locals (`m`, `o`); the later `char *p = &h`
  pointer-local path remains fail-closed as pointer/global publication.
- `src/pr68185.c`: first owner is the RV64 local-memory consumer. First missing
  authority fact is scalar local publication/consumption for global-derived
  locals (`h = o = z`, `g = w`), with separate edge-store-slot local
  destination evidence at `%t38.phi` left fail-closed as publication ordering.
- `src/pr68321.c`: first owner is the RV64 local-memory consumer, but the first
  row-specific authority is not the shared scalar publication family: fresh
  evidence shows edge-store-slot local destination publication for `%t17.phi`
  and a local-array/frame-slot read feeding `u = g[0]`. Leave it fail-closed
  under publication-ordering/local aggregate ownership rather than widening the
  mixed scalar path.
- `src/pr70005.c`: first owner is the RV64 local-memory consumer. First missing
  authority fact is scalar frame-slot local publication/consumption across many
  global-derived local temporaries; prepared addressing already records
  frame-slot base-plus-offset accesses, so this is not a direct global-symbol
  local-memory residual.

## Suggested Next

Delegate an implementation packet for the shared scalar frame-slot
local-memory consumer family: consume explicit prepared frame-slot
base-plus-offset local access facts for scalar locals whose stores/loads are
already published, using one of `src/pr58431.c`, `src/pr68185.c`, or
`src/pr70005.c` as the proving row. Keep `src/pr68321.c` out of that packet
unless a separate publication-ordering/local-aggregate owner is selected.

## Watchouts

Do not broaden direct global-symbol local-memory policy from idea 631. The
direct global accesses in these rows already have scalar global-symbol
base-plus-offset records; the current stop is ordinary local frame-slot use in
the RV64 consumer. Pointer-to-global locals (`pr57861`, `pr58431`) and
edge-store/local-array evidence (`pr68185`, `pr68321`) should remain
fail-closed unless the supervisor deliberately splits them to pointer
freshness, publication ordering, or aggregate ownership.

## Proof

Ran the supervisor-selected proof command:

`cmake --build --preset default && ALLOWLIST=build/agent_state/640_step1_mixed_publication.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: build succeeded, focused backend probe exited nonzero with
`total=5 passed=0 failed=5`. This is sufficient for the evidence-refresh
packet because the row classifications are based on the preserved
`test_after.log`, per-case logs under `build/rv64_gcc_c_torture_backend/`, and
fresh prepared dumps under `build/agent_state/640_step1_*.prepared.txt`.
