Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family repaired the vector alloca
producer gap for `src/20050604-1.c`.

Implemented:

- Fixed LLVM vector type parsing for local allocas such as `<4 x i16>` and
  `<4 x float>`.
- Published each fixed vector alloca as a local array source object with scalar
  lane slots, lane count, lane size, total object extent, alignment, and
  available source-object status.
- Used deterministic vector lane slot names with `.lane.N` suffixes so a vector
  object like `%lv._clit_` cannot collide with a sibling source object named
  `%lv._clit_.1`.
- Added focused BIR coverage:
  `expect_local_vector_alloca_publishes_lane_slots_and_source_object`.

Representative result:

- `src/20050604-1.c` moved past `alloca local-memory semantic family` and now
  stops in `store local-memory semantic family`, matching the expected
  downstream vector-store boundary after producer publication.

## Suggested Next

Recommended next packet: inspect and repair the newly exposed vector store
local-memory boundary for `src/20050604-1.c`, starting from the zero-vector
stores to `%lv._clit_` / `%lv._clit_.1` and keeping vector load/arithmetic or
global vector stores separate unless a shared store producer fact is required.

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
publication. The first exposed downstream gap is now vector store local-memory;
do not route it back through alloca naming or RV64/MIR recovery.

## Proof

Proof recorded in `test_after.log`.

Commands run:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed: `345/345` backend tests.
- `ALLOWLIST=build/agent_state/557_step13_20050604.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  failed the selected row after moving it from `alloca local-memory semantic
  family` to `store local-memory semantic family`.

Additional check:

- `git diff --check` passed.
