Status: Active
Source Idea Path: ideas/open/652_prepared_incoming_stack_formal_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Producer Authority Boundary

# Current Packet

## Just Finished

Step 1 located the producer/prealloc authority boundary for
`src/20001017-1.c`.

Current producer owner:
`src/backend/prealloc/formal_publications.cpp::plan_prepared_formal_publication`
chooses `IncomingStackToHome` from BIR ABI facts plus the prepared value home.
It publishes only the action and `PreparedValueHome` pointer. The available
home facts are produced by
`src/backend/prealloc/regalloc/value_homes.cpp::classify_prepared_value_home`,
with RV64 fixed stack-passed scalar homes selected by
`find_rv64_stack_passed_fixed_formal_home_slot`.

Current facts available in the fresh probe
`build/agent_state/652_step1_authority_boundary/20001017-1.prepared.txt`:
caller-side call argument plans and ABI bindings name outgoing stack
destinations for `bug` (`arg9 -> stack+0`, `arg11 -> stack+8`,
`arg12 -> stack+16`), but callee formal homes name local frame slots
(`%p.fdB -> slot#10/offset40`, `%p.C -> slot#12/offset48`,
`%p.fdC -> slot#11/offset44`). There is no published fact tying the callee
formal to its incoming caller-stack byte offset independently of the local
home. The current object probe still fails closed with the missing explicit
incoming stack formal authority diagnostic.

Classification: this is an upstream publication/model gap in
producer/prealloc formal publication, not a latent RV64 consumption gap.

## Suggested Next

Execute Step 2: add the smallest producer/prealloc representation for explicit
incoming caller-stack formal authority. The natural repair family is to extend
formal-publication data with an incoming stack offset/address fact that is
derived upstream from target ABI/prealloc policy, exposed through focused
producer/prealloc tests, and kept distinct from `PreparedValueHome` local
frame-slot offsets.

## Watchouts

- Do not reintroduce RV64 helpers that compute incoming offsets by walking
  `function.params`, applying ABI size/alignment, or adding
  `stack_frame_bytes`.
- Keep `IncomingStackToHome` plus callee local home distinct from explicit
  caller-stack incoming authority.
- Positive RV64 coverage should wait until the producer/prealloc fact exists;
  producer/prealloc coverage should prove the authority first.
- Do not repurpose `PreparedValueHome::offset_bytes` for incoming stack
  authority; in the current stack-passed formal rows it is the callee local
  frame-slot offset.
- The caller-side `PreparedCallPlan` has outgoing destination stack offsets for
  call arguments, but those rows are callsite facts, not callee formal incoming
  authority.

## Proof

Fresh Step 1 probes:
`build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/20001017-1.c > build/agent_state/652_step1_authority_boundary/20001017-1.prepared.txt 2> build/agent_state/652_step1_authority_boundary/20001017-1.prepared.err`

`build/c4cll --target riscv64-linux-gnu --codegen obj -o build/agent_state/652_step1_authority_boundary/20001017-1.o tests/c/external/gcc_torture/src/20001017-1.c > build/agent_state/652_step1_authority_boundary/20001017-1.obj.out 2> build/agent_state/652_step1_authority_boundary/20001017-1.obj.err`

Object probe result: expected fail-closed diagnostic
`unsupported_param_home: RV64 object route requires explicit prepared incoming stack formal authority before consuming stack-passed scalar formal homes`.

Validation command written to `test_after.log`:
`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_formal_publications|backend_prepare_frame_stack_call_contract)$') > test_after.log 2>&1`

Result: PASS, 2/2 focused producer/prealloc tests passed.
