Status: Active
Source Idea Path: ideas/open/676_rv64_pointer_global_local_publication_runtime_contract.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Decide Valid Object Versus Fresh Implementation Owner

# Current Packet

## Just Finished

Step 2 converted
`backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
from a stale expected-failure object wrapper into a positive RV64 object-route
contract. The row now requires RV64 ELF emission and a stable live-load object
byte sequence for the pointer reload plus short load/store/reload path. The
contract rationale is recorded in
`build/agent_state/676_step2_contract_update/summary.md`.

## Suggested Next

Review whether Step 2 closes the runbook route or delegate the next plan step
if the supervisor wants broader validation before lifecycle review.

## Watchouts

- Do not accept `test_baseline.new.log` while it contains candidate-only
  failures.
- Compare by stable test name, not numeric row id.
- Do not reopen closed ideas unless fresh evidence contradicts their closure
  notes.
- Do not change unsupported markers, allowlists, timeouts, runtime policy, or
  baseline accounting.
- Step 2 intentionally changes only the stale test expectation for a case that
  Step 1 already proved links and runs correctly under qemu.
- Host `objdump -dr` cannot disassemble this RISC-V object here
  (`architecture UNKNOWN`); use `riscv64-linux-gnu-objdump -dr` for readable
  object evidence.
- The call-arg row is split to
  `ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md`;
  do not bundle it into this packet.

## Proof

Ran the delegated proof into `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection|backend_cli_riscv64_pointer_global_local_publication'`.
Build and focused CTest passed. The updated live-load row now asserts positive
RV64 object emission, and the nearby existing publication object row still
passes.
