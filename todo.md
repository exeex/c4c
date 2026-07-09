Status: Active
Source Idea Path: ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove breadth and close-readiness

# Current Packet

## Just Finished

Completed Step 5 breadth and close-readiness proof by rerunning the Step 1
outgoing-stack argument diagnostic row set and comparing first-owner buckets
before and after the producer/consumer changes.

Artifact:
`build/agent_state/624_step5_breadth_close_readiness/`

Result:

- Step 1 producer-owned rows now expose explicit prepared destination facts:
  `20000808-1.c`, odd `931004-*`, `931031-1.c`, and `950607-2.c` publish
  `dest_stack_offset` and `dest_stack_size` when they reach the prepared call
  boundary.
- Stack-copy aggregate rows also publish matching transport destination facts:
  `transport_dest_stack_offset` and `transport_dest_stack_size` are present for
  the byval stack-copy arguments in `20000808-1.c` and `950607-2.c`.
- Current RV64 object diagnostics for the former producer-owned rows now remain
  at the broader `unsupported_call_abi` ordinary same-module call ABI/result
  gate, not at missing outgoing destination authority.
- Even `931004-*` rows remain semantic local-memory guards before prepared
  handoff, and `pr69447.c` remains the scalar outgoing-stack guard with
  `dest_stack_offset=0` and `dest_stack_size=8`.
- No expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting change was used as progress.

## Suggested Next

Recommend supervisor close-readiness review for idea 624. The prepared
outgoing-stack destination-offset route is complete; any next implementation
should be a narrower follow-up owned by the residual bucket rather than an
expansion of this idea.

## Watchouts

- Residual RV64 object failures for `20000808-1.c`, odd `931004-*`,
  `931031-1.c`, `950607-2.c`, and `pr69447.c` are classified as ordinary
  same-module call ABI/result support, outside idea 624.
- Even `931004-*` rows remain semantic local-memory guards and should stay out
  of an outgoing-stack destination implementation route.
- `20000808-1.c` still exposes carrier-alias/select residuals in prepared
  diagnostics; keep that out of this implementation route unless a separate
  source idea owns it.

## Proof

Delegated proof command:
`cmake --build build --target c4cll > test_after.log 2>&1`

Result: passed, preserved in `test_after.log`.

Diagnostic refresh command:
`./build/c4cll -I tests/c/external/gcc_torture --dump-bir --target riscv64-linux-gnu <gcc torture row>`
`./build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu <gcc torture row>`
`./build/c4cll -I tests/c/external/gcc_torture --codegen asm --target riscv64-linux-gnu <gcc torture row> -o <artifact>/asm/<row>/out.s`
`cmake -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake ...`

Representative diagnostic output:
`build/agent_state/624_step5_breadth_close_readiness/`
