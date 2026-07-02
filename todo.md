Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family completed the GEP-family
producer repair for `src/20000717-4.c`'s boundary.

Implemented producer-side publication for the general shape where a constant
global GEP chain reaches a scalar-array subobject through `global_pointer_slots`
and a later dynamic scalar GEP indexes that subobject. The repair walks the
global root layout at the published byte offset, confirms the scalar element
type, and publishes `DynamicGlobalScalarArray` /
`GlobalStaticGepAuthorityRecord` facts before the pointer-array fallback.

Focused BIR coverage added:

- `expect_global_struct_member_scalar_array_dynamic_gep_publishes_authority`
  models `%struct.slot = type { [6 x i32] }`,
  `%struct.Root = type { i32, [4 x %struct.slot] }`, a global `@s`, a constant
  GEP chain to the nested `[6 x i32]` member, then a dynamic `i32` GEP and load.
- The test pins available `DynamicGlobalScalarArray` authority, global identity,
  dynamic range metadata, and materialized `LoadGlobalInst` behavior.

RV64 representative result:

- `src/20000717-4.c` moved off semantic `gep local-memory` admission.
- It now fails downstream in the object route with
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.

## Suggested Next

Recommended next packet: continue Step 13 or advance to Step 14 by selecting
one remaining semantic local-memory representative still in the producer lane,
preferably `src/20000519-1.c` (`scalar/local-memory`) or `src/20050604-1.c`
(`alloca local-memory`), and inspect its missing BIR fact boundary before code
changes.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

Do not route this as a local-slot repair: the representative source uses a
global object, but the shared semantic failure bucket reported
`gep local-memory`; this packet fixed only the producer fact gap for the
constant-global-chain-to-scalar-array shape.

Keep downstream object-route failures out of this producer packet:
`src/20000314-1.c`, `src/20001026-1.c`, and now `src/20000717-4.c` have moved
off semantic local-memory admission and should not be absorbed back into this
source idea without supervisor/lifecycle direction.

## Proof

Proof log: `test_after.log`.

Commands run:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
  passed: `345/345` backend tests.
- `ALLOWLIST=build/agent_state/557_step13_20000717.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  appended to `test_after.log`; result `0/1` passed, `1/1` failed, but the row
  moved from semantic `gep local-memory` to downstream
  `ambiguous_non_parallel_multi_source_stack_destination`.

Inspected case log:
- `build/rv64_gcc_c_torture_backend/src_20000717-4.c/case.log`
