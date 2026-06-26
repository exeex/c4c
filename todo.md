Status: Active
Source Idea Path: ideas/open/394_rv64_same_module_sret_callee_home_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Callee Home Publication Rule

# Current Packet

## Just Finished

Step 2 classified the narrow RV64 callee `sret_param` home-publication rule.

Selected route:
- Treat stack-homed RV64 `sret_param` pointer publication as an entry formal
  publication.
- Use prepared formal/home facts plus RV64 ABI register classification to
  authorize storing the incoming sret pointer register into the prepared
  `%ret.sret` stack home.
- Keep the later pointer-value return store as a consumer of the prepared
  `%ret.sret` home; do not make it infer or repair missing entry publication.

Authorizing facts:
- BIR formal has `is_sret=true`, `type=Ptr`, pointer-sized storage, and ABI
  says passed in an integer register, not on stack.
- RV64 formal-register numbering resolves that sret formal to incoming `a0`.
- The prepared value home is a stack slot for the same formal value.
- The prepared frame slot matches the home slot/function/offset/size/align.
- The stack object behind the slot has `source_kind=sret_param`, `type=Ptr`,
  `address_exposed=yes`, `requires_home_slot=yes`, and
  `permanent_home_slot=yes`.
- The home offset is inside the prepared frame and encodable by an RV64 store.

Implementation target for the next packet:
- Extend `plan_prepared_formal_publication`, which currently returns
  `NoPublication` for `formal.is_sret`, so the supported sret shape produces an
  explicit `IncomingRegisterToHome` publication.
- Extend RV64 `make_rv64_formal_entry_home_fragment`/admission guards to consume
  only that supported sret publication and emit the prologue store from `a0` to
  the prepared home.

Detailed guard notes are recorded in
`build/agent_state/394_step2_callee_home_rule.log`.

## Suggested Next

Execute Step 3 by adding focused backend coverage for the selected rule. Cover
the accepted RV64 stack-homed `sret_param` publication from incoming `a0`, plus
fail-closed variants for malformed ABI/home/frame-slot/object facts.

## Watchouts

- Keep this separate from caller-side `memory_return` address materialization;
  Step 1 already confirmed the caller passes the intended address in `a0`.
- Keep this separate from idea 393 aggregate `va_arg` cursor stride.
- Fail closed rather than saving arbitrary `a0`: require the explicit sret
  formal, ABI, prepared stack home, frame slot, and `sret_param` object facts.
- Avoid hard-coding `920908-1.c`, callee `f`, literal stack offsets, or the
  final return-store instruction sequence.

## Proof

Read-only classification only. No build, representative runner, or CTest command
was run for Step 2, so `test_after.log` remains the Step 1 representative
evidence log.

Read-only checks:
- `plan.md` Step 2
- `todo.md` Step 1 evidence
- `build/agent_state/394_step1_analysis.log`
- `src/backend/prealloc/formal_publications.{hpp,cpp}`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Analysis artifact:
- `build/agent_state/394_step2_callee_home_rule.log`
