Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family repaired the vector store
producer gap for `src/20050604-1.c`.

Implemented:

- Lowered fixed scalar-vector zeroinitializer stores into already-published
  local vector source objects by writing each scalar lane slot directly.
- Preserved deterministic `.lane.N` lane-slot naming and local-slot memory
  provenance on the emitted lane stores.
- Added focused BIR coverage:
  `expect_local_vector_store_writes_lane_slots_and_source_facts`.

Representative result:

- `src/20050604-1.c` moved past `store local-memory semantic family` and now
  stops in `load local-memory semantic family`, matching the expected
  downstream vector-load boundary after vector lane-store publication.

## Suggested Next

Recommended next packet: inspect and repair the newly exposed vector load
local-memory boundary for `src/20050604-1.c`, starting from loads of the local
vector source objects after the zero-vector lane stores and keeping vector
arithmetic or global vector stores separate unless a shared load producer fact
is required.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

Keep downstream object-route failures out of this producer packet:
`src/20000314-1.c`, `src/20001026-1.c`, `src/20000717-4.c`, and now
`src/20000519-1.c` have moved off semantic local-memory admission and should
not be absorbed back into this source idea without supervisor/lifecycle
direction.

`src/20050604-1.c` may expose further semantic gaps after vector alloca
and store publication. The next exposed downstream gap is now vector load
local-memory; do not route it back through alloca naming, lane-store lowering,
or RV64/MIR recovery.

## Proof

Proof recorded in `test_after.log`.

Commands run:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed: `345/345` backend tests.
- `ALLOWLIST=build/agent_state/557_step13_20050604.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  failed the selected row after moving it from `store local-memory semantic
  family` to `load local-memory semantic family`.

Additional check:

- `git diff --check` passed.
