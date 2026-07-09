Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Residual Row

# Current Packet

## Just Finished

Step 1 from `plan.md` is complete as an evidence-only refresh of the
`src/20001017-1.c` RV64 residual row.

- Allowlist: `build/agent_state/644_step1_20001017_1.allowlist`, containing
  `src/20001017-1.c`.
- Proof log: `test_before.log`.
- Case log:
  `build/rv64_gcc_c_torture_backend/src_20001017-1.c/case.log`.
- Work directory result: only `case.log` was produced; no object or binary was
  produced because object compile failed.
- Result: nonzero due to the one-row residual. The build was up to date, and
  one row failed.
- Current first owner:
  `[RV64_C4C_OBJ_COMPILE_FAIL]`, diagnostic
  `unsupported_call_abi: RV64 object route requires supported ordinary same-module call ABI/result lowering; function=main; block=entry; block_index=0; instruction_index=0; callee=bug; args=13; planned_args=13; result=none`.
- Interpretation: `unsupported_call_abi` remains the active boundary for idea
  644. Branch stack clobber-safety did not return as first owner.

## Suggested Next

Start Step 2 by classifying the active ordinary same-module call ABI/result
lowering boundary from the refreshed residual, using the existing prepared
dumps as context before choosing a semantic implementation packet.

## Watchouts

- Existing prior prepared dumps available for Step 2:
  `build/agent_state/635_step1_20001017-1.prepared.txt` and
  `build/agent_state/635_step4_src_20001017-1.c_prepared_dump.txt`.
- Do not treat branch clobber-safety as the active owner unless fresh evidence
  shows it has returned.
- Do not reconstruct parameter placement from source syntax, stack offsets, or
  final assembly.
- Do not change expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting as capability progress.

## Proof

Supervisor-provided Step 1 probe; not rerun by this evidence-only executor
packet:

```sh
rm -f test_before.log test_after.log && (cmake --build --preset default && ALLOWLIST=build/agent_state/644_step1_20001017_1.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_before.log 2>&1
```

Result: nonzero due to the one-row residual. Canonical proof artifact for this
packet is `test_before.log`; row detail is in
`build/rv64_gcc_c_torture_backend/src_20001017-1.c/case.log`.
