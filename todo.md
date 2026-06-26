Status: Active
Source Idea Path: ideas/open/405_prepared_local_aggregate_slot_layout_facts.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Producer Slot Layout Publication

# Current Packet

## Just Finished

Step 2 repaired producer-side local byte-storage overlay extent publication.

Semantic aggregate slot declaration now records explicit scalar slot sizes,
and local-memory lowering publishes a covering extent for the base byte slot
only when a multi-byte scalar access actually overlays byte storage. The repair
uses the carried byte-element slot list or byte-storage layout facts from the
producer path; it does not special-case filenames, fields, or RV64 object
emission, and ordinary byte-array byte accesses remain byte-sized.

Before/after facts:

- `build/agent_state/405_step2_dumps/20020225-2.prepared-bir.txt` now shows
  `%lv.a.0` as `type=i8 size=8 align=8`, while `%lv.a.1..7` remain
  `size=1`. The double store and i32 store/load select `frame_slot=#0` with
  sizes 8/4/4 and `range_verdict=proven_in_bounds`.
- `build/agent_state/405_step2_dumps/ieee_mul-subnormal-single-1.prepared-bir.txt`
  now shows `u2f` and `f2u` `%lv.u.0` as `type=i8 size=4 align=4`, while
  `%lv.u.1..3` remain `size=1`. The i32/float overlay accesses select the
  covering frame slot and are `proven_in_bounds`.
- Focused producer/backend tests cover the BIR publication and prepared
  frame-slot selection facts, and the existing ordinary byte-array route
  `backend_codegen_route_riscv64_prepared_local_array_i8_element_access`
  still passes.

## Suggested Next

The next implementation packet should target the remaining RV64 object-route
local-memory boundary. The allowlisted torture probe no longer shows the old
one-byte/proven-out-of-bounds prepared fact family, but both representatives
still stop in RV64 object lowering with `unsupported_local_memory_access`.

## Watchouts

- The RV64 probe cases still fail, but at a later object-route boundary:
  `src/20020225-2.c` reports the generic prepared local-memory addressing
  support message, and `src/ieee/mul-subnormal-single-1.c` reports the RV64
  object-route width support message. Do not reclassify these as the repaired
  one-byte frame-slot extent bug without checking the prepared dumps.
- Keep the current use-driven publication shape. Widening every one-byte slot
  at declaration time regresses ordinary byte-array layout and was avoided.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && mkdir -p build/agent_state/405_step2_dumps
&& printf '%s\n' 'src/20020225-2.c' 'src/ieee/mul-subnormal-single-1.c' >
build/agent_state/405_step2_local_aggregate.allowlist.txt && { ALLOWLIST=build/agent_state/405_step2_local_aggregate.allowlist.txt
STOP_ON_FAILURE=0 VERBOSE_FAILURES=1
scripts/check_progress_rv64_gcc_c_torture_backend.sh >
build/agent_state/405_step2_local_aggregate_probe.log 2>&1 || true; } && rg -n
'unsupported_local_memory_access|unsupported_instruction_fragment|unsupported
RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:'
build/agent_state/405_step2_local_aggregate_probe.log && ctest --test-dir
build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: acceptance-green for this Step 2 producer repair. The build completed,
the allowlisted probe log was written to
`build/agent_state/405_step2_local_aggregate_probe.log`, and the backend CTest
subset wrote `test_after.log` with `100% tests passed, 0 tests failed out of
326`.

The allowlisted RV64 torture probe still reports `RV64_C4C_OBJ_COMPILE_FAIL`
for both allowlisted cases, but the representative prepared dumps show the
wide local overlay accesses now select covering frame slots with
`range_verdict=proven_in_bounds`. The remaining probe failures are later
object-route boundaries, not the repaired one-byte frame-slot extent family.

Additional focused validation passed:
`ctest --test-dir build -j --output-on-failure -R
'^(backend_lir_to_bir_notes|backend_prepare_stack_layout|backend_codegen_route_riscv64_prepared_local_array_i8_element_access)$'`.
