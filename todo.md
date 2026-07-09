Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Prepared Incoming Stack Formal Authority Path

# Current Packet

## Just Finished

Step 3 removed the rejected RV64 consumer-side incoming stack formal
reconstruction from `object_emission.cpp` and
`prepared_local_memory_emit.cpp`.

The available prepared formal publication model records
`IncomingStackToHome` plus the callee local stack-slot home, but it does not
publish an explicit caller-stack incoming offset/address authority distinct
from that local spill-slot home. RV64 now fails closed for stack-passed scalar
formal homes with an explicit missing-authority diagnostic instead of loading
`stack_frame_bytes + inferred_incoming_offset`.

Focused RV64 object coverage was adjusted away from literal `sp + 64`
positive assertions and now covers the missing explicit incoming-stack
authority path.

## Suggested Next

Next packet should publish explicit prepared incoming stack formal authority
from the producer/prealloc side. That fact needs to identify the caller-stack
incoming byte offset/address for each stack-passed scalar formal separately
from the callee's local spill-slot/home record.

## Watchouts

- Do not build on helpers that compute `incoming_offset` by walking
  `function.params` and then add `stack_frame_bytes`; `review/reviewA.md`
  marks that route as high-severity drift.
- Positive consumer coverage should wait until producer/prealloc publishes an
  explicit incoming-stack authority fact; do not reintroduce literal expected
  load offsets such as `sp + 64` as the proof.
- The current `src/20001017-1.c` residual is now classified as a
  producer/prealloc publication gap, not an RV64 consumer inference task.
- The local address helper intentionally reuses
  `prepared_frame_slot_address_call_argument_offset`, so missing prepared
  addressing/frame-plan authority remains fail-closed instead of falling back to
  source syntax or final assembly.
- Stack-argument local frame-address publication must account for an active
  outgoing call-stack adjustment; the focused test checks this with an adjusted
  `s2` materialization before storing the stack argument.
- The prior caller-side local frame-address residual is gone in the linked
  `main`: it now emits `addi t0, sp, 0x10`, adjusted `addi t0, sp, 0x28`,
  `addi s1, sp, 0x18`, and `addi s2, sp, 0x20` before consuming those pointer
  sources as register or stack call arguments.
- The current callee-side residual now stops at object-route admission with the
  missing explicit incoming-stack formal authority diagnostic.

## Proof

Exact delegated proof command was run and wrote `test_after.log`.

Build and selected CTest subset passed:
`backend_riscv_object_emission`,
`backend_prepare_frame_stack_call_contract`,
`backend_prepared_lookup_helper`,
`backend_prealloc_call_boundary_classification`,
`backend_prepared_object_consumer_contract`, and
`backend_call_boundary_effect_plan`.

The final allowlisted RV64 GCC torture progress check failed only for
`src/20001017-1.c`, with:
`unsupported_param_home: RV64 object route requires explicit prepared incoming
stack formal authority before consuming stack-passed scalar formal homes`.
This is the classified producer/prealloc publication gap, but the exact
delegated shell command exited non-zero.
