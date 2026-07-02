Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family completed an inspection
subpacket for the remaining alloca-family representative,
`src/20050604-1.c`.

Boundary found:

- `foo` lowers compound literals to static vector local allocas:
  `%lv._clit_ = alloca <4 x i16>, align 8` and
  `%lv._clit_.1 = alloca <4 x float>, align 16`.
- `lower_local_memory_alloca_inst` currently admits only scalar/function-pointer
  local slots, fixed scalar arrays like `[N x i64]`, and declared aggregate
  layouts via `declare_local_aggregate_slots`.
- LLVM vector type spellings such as `<4 x i16>` and `<4 x float>` are not
  accepted by `lower_scalar_or_function_pointer_type`,
  `parse_local_array_type`, or aggregate layout lookup, so the function fails
  during alloca lowering before the later vector zero store, lane GEP/store,
  vector load, vector add, or global vector store paths can run.
- The repair boundary is therefore vector local-object alloca publication:
  treat fixed LLVM vector allocas as local memory objects with scalar lane
  slots/source metadata, rather than adding a `20050604-1.c` shaped exception
  or moving the missing fact into RV64/MIR.

## Suggested Next

Recommended next packet: add focused BIR coverage and implement vector alloca
producer publication for fixed scalar vectors.

Suggested focused test gap:
`expect_local_vector_alloca_publishes_lane_slots_and_source_object`.

The test should model at least one alloca such as `<4 x i16>` and assert that
the semantic producer publishes deterministic lane local slots with the vector
object extent, lane type/count/size, alignment, and source-object metadata
needed by subsequent local-memory GEP/load/store admission. It should not rely
on RV64 behavior or fold vector arithmetic/object lowering into the alloca
repair.

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
publication, especially vector stores/loads or vector arithmetic. Keep those
separate unless the alloca repair directly requires a shared, producer-owned
local-memory fact.

## Proof

Inspection packet only; no build required and `test_after.log` was preserved.

Commands/evidence used:

- Existing case log:
  `build/rv64_gcc_c_torture_backend/src_20050604-1.c/case.log`, which reports
  `foo` failing in `alloca local-memory semantic family`.
- LLVM inspection:
  `./build/c4cll --codegen llvm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20050604-1.c -o /tmp/20050604-1.ll`.
- Semantic BIR dump attempt:
  `./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20050604-1.c`, which still fails at the same alloca family before producing BIR.

Post-repair RV64 representative proof command for the supervisor:

- `ALLOWLIST=build/agent_state/557_step13_20050604.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
