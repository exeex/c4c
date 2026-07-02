Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family repaired the fixed local
vector load producer gap for `src/20050604-1.c`.

Implemented:

- Lowered fixed scalar-vector loads from already-published local vector source
  objects into deterministic scalar lane loads.
- Preserved deterministic `%loaded.lane.N` result naming and local-slot memory
  provenance on the emitted lane loads.
- Added focused BIR coverage:
  `expect_local_vector_load_reads_lane_slots_and_source_facts`.

Representative result:

- `src/20050604-1.c` was rechecked. It still reports
  `load local-memory semantic family`, but the repaired local loads match the
  direct `%lv._clit_` / `%lv._clit_.1` shape. The next visible boundary in
  `foo` is vector load admission from global union-member addresses
  `%t13` / `%t32` after the local vector loads.

## Suggested Next

Recommended next packet: inspect the remaining vector load local-memory
boundary for `src/20050604-1.c`, starting from global union-member loads like
`load <4 x i16>, ptr %t13` and `load <4 x float>, ptr %t32`, and keep vector
arithmetic/global vector stores separate unless a shared load producer fact is
required.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

Keep downstream object-route failures out of this producer packet:
`src/20000314-1.c`, `src/20001026-1.c`, `src/20000717-4.c`, and now
`src/20000519-1.c` have moved off semantic local-memory admission and should
not be absorbed back into this source idea without supervisor/lifecycle
direction.

`src/20050604-1.c` may expose further semantic gaps after vector alloca,
store, and local-load publication. The next exposed downstream gap still uses
the broad load local-memory note, but the visible LLVM shape has moved on to
global vector loads from the union fields; do not route it back through alloca
naming, lane-store lowering, direct local-load lowering, or RV64/MIR recovery.

## Proof

Proof recorded in `test_after.log`.

Commands run:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed: `345/345` backend tests.
- `ALLOWLIST=build/agent_state/557_step13_20050604.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  failed the selected row with `load local-memory semantic family`; inspection
  of the LLVM route shows the next visible load boundary is global vector load
  admission after the repaired local vector loads.

Additional check:

- `git diff --check` passed.
