Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family repaired the fixed
global/global-member vector load producer gap for `src/20050604-1.c`.

Implemented:

- Lowered fixed scalar-vector loads from direct globals or GEP-published global
  addresses into deterministic scalar lane `LoadGlobalInst`s.
- Preserved deterministic `%loaded.lane.N` result naming, global LinkNameId
  identity, byte offsets, and lane-sized alignment on emitted lane loads.
- Added focused BIR coverage:
  `expect_global_vector_load_reads_lane_offsets_and_identity`.

Representative result:

- `src/20050604-1.c` was rechecked. It moved from the remaining
  `load local-memory semantic family` boundary to `scalar-binop semantic
  family` after the global vector loads from `%t13` / `%t32` were admitted.

## Suggested Next

Recommended next packet: inspect the new scalar-binop boundary for
`src/20050604-1.c` after fixed vector alloca/store/local-load/global-load
producer admission. Keep vector arithmetic repair separate from global vector
stores unless the next failure proves they share the same semantic producer
fact.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

Keep downstream object-route failures out of this producer packet:
`src/20000314-1.c`, `src/20001026-1.c`, `src/20000717-4.c`, and now
`src/20000519-1.c` have moved off semantic local-memory admission and should
not be absorbed back into this source idea without supervisor/lifecycle
direction.

`src/20050604-1.c` has now moved through vector alloca, zeroinitializer store,
direct local vector load, and global/global-member vector load producer
admission. Do not route the next scalar-binop failure back through alloca
naming, lane-store lowering, direct local-load lowering, global-load lowering,
or RV64/MIR recovery.

## Proof

Proof recorded in `test_after.log`.

Commands run:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed: `345/345` backend tests.
- `ALLOWLIST=build/agent_state/557_step13_20050604.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  failed the selected row after moving it to `scalar-binop semantic family`.

Additional check:

- `git diff --check` passed.
