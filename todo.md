Status: Active
Source Idea Path: ideas/open/406_rv64_object_route_residual_local_memory_boundaries.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Representatives And Route Residuals

# Current Packet

## Just Finished

Completed Step 4 classification/proof for the two delegated representatives.
The current probe shows neither representative still fails with
`unsupported_local_memory_access`, and neither reports the old local-memory
diagnostics repaired by Steps 2 and 3.

`src/20020225-2.c` now stops at
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering`, which routes to
`ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.

`src/ieee/mul-subnormal-single-1.c` now stops at
`unsupported_stack_frame: RV64 object route requires supported prepared
callee-saved GPR save slots`, which routes to
`ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`.

No same-family local-memory boundary remains in scope for idea 406 based on
the delegated representative probe.

## Suggested Next

Ask the plan owner to perform lifecycle review for idea 406 and decide whether
to close the active plan or split any residual notes into the already-open
owner ideas above.

## Watchouts

- The representatives still do not object-compile, so there is no qemu runtime
  comparison for this packet.
- The residual failures are distinct owner families, not acceptance evidence
  for those owners.
- 406 appears complete from this Step 4 representative proof; lifecycle close
  remains a plan-owner decision.

## Proof

Ran the supervisor-selected Step 4 proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/406_step4_dumps && printf '%s\n' 'src/20020225-2.c' 'src/ieee/mul-subnormal-single-1.c' > build/agent_state/406_step4_route_residuals.allowlist.txt && { ALLOWLIST=build/agent_state/406_step4_route_residuals.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/406_step4_route_residuals_probe.log 2>&1 || true; } && rg -n 'unsupported_local_memory_access|unsupported_stack_frame|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/406_step4_route_residuals_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build passed; the RV64 gcc_torture probe ran both allowlisted
representatives and reported two expected compile failures, both at later
non-local-memory boundaries; backend CTest passed with 326/326 tests passing.
Canonical proof log: `test_after.log`. Supporting packet artifacts:
`build/agent_state/406_step4_route_residuals_probe.log`,
`build/agent_state/406_step4_route_residuals.allowlist.txt`, and
`build/agent_state/406_step4_dumps/`.
