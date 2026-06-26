Status: Active
Source Idea Path: ideas/open/406_rv64_object_route_residual_local_memory_boundaries.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Valid Local-Memory Width Admission

# Current Packet

## Just Finished

Completed the Step 3 repair for valid F32 prepared local-memory width
admission. RV64 object emission now treats F32 as a 4-byte prepared local
memory type and lowers F32 local loads/stores through the existing prepared
frame-slot or pointer-value base-plus-offset validators, preserving explicit
size, alignment, range, signed-offset, frame-slot, and pointer-base checks.

Focused backend coverage now covers normal F32 local store/load pairs plus the
two explicit 4-byte overlay directions represented by the IEEE case family:
I32 store followed by F32 load, and F32 store followed by I32 load. Existing
invalid-width negative coverage was moved from newly valid F32 to F128/16-byte
local memory.

Representative outcome: `src/ieee/mul-subnormal-single-1.c` advanced from the
F32 local-memory width diagnostic to
`unsupported_stack_frame: RV64 object route requires supported prepared
callee-saved GPR save slots`.

`src/20020225-2.c` remains at its later Step 2 boundary:
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering`.

## Suggested Next

Use Step 4 to classify the remaining representative outcomes and route the
`mul-subnormal-single-1.c` unsupported stack-frame/callee-save boundary to its
own owner if it is outside this local-memory plan.

## Watchouts

- The F32 repair intentionally admits only explicit prepared 4-byte local
  memory facts; it does not infer union layout or fabricate missing frame-slot
  facts.
- F32/F64 local-memory lowering uses FPR load/store encodings; integer
  same-width overlay lanes still use the scalar GPR route.
- `mul-subnormal-single-1.c` still does not object-compile, but the current
  failure is no longer the local-memory width family.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/406_step3_dumps && printf '%s\n' 'src/20020225-2.c' 'src/ieee/mul-subnormal-single-1.c' > build/agent_state/406_step3_f32_local_memory.allowlist.txt && { ALLOWLIST=build/agent_state/406_step3_f32_local_memory.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/406_step3_f32_local_memory_probe.log 2>&1 || true; } && rg -n 'unsupported_local_memory_access|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/406_step3_f32_local_memory_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build passed; RV64 gcc_torture probe completed with both allowlisted
representatives still failing but on distinct later boundaries
(`20020225-2.c` at `unsupported_instruction_fragment`,
`mul-subnormal-single-1.c` at `unsupported_stack_frame` for prepared
callee-saved GPR save slots); backend CTest passed. Canonical proof log:
`test_after.log`. Supporting packet artifacts:
`build/agent_state/406_step3_f32_local_memory_probe.log`,
`build/agent_state/406_step3_f32_local_memory.allowlist.txt`, and
`build/agent_state/406_step3_dumps/`.
