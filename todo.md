Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Reclassify Remaining Param-Home Rows

# Current Packet

## Just Finished

Step 6 of `plan.md` routed RV64 object-emission admission through the
published prepared stack-slot homes for ordinary stack-passed scalar formals.

Implementation boundary:

- `object_emission.cpp` now accepts a non-byval, non-sret, non-varargs formal
  stack-slot home only when the BIR formal has explicit
  `param.abi->passed_on_stack` authority, no register-passed ABI, an Integer
  or Sse scalar class, matching type/size/alignment, and a prepared home that
  matches the frame-slot and `regalloc.spill_slot` stack object facts.
- RV64 still rejects missing, offsetless, mismatched, memory-class, F128,
  address-exposed, permanent-home, or non-regalloc stack homes with
  `unsupported_param_home`.
- Stack-passed formal homes are consumed as already-in-place incoming storage:
  RV64 does not derive offsets from argument index, parameter/source spelling,
  or call shape, and it does not synthesize an entry register-store fragment
  for them.
- The existing register-passed scalar, byval, sret, and FPR-register formal
  home paths were left unchanged except for the new stack-passed branch being
  checked first for explicit `passed_on_stack` homes.

Representative before/after facts for `tests/c/external/gcc_torture/src/20001017-1.c`:

- Step 6 prepared dump:
  `build/agent_state/512_step6_rv64_consumption/20001017-1/prepared.txt`.
- Caller ABI bindings for indexes 8-12 remain explicit stack offsets `0`,
  `8`, `16`, `24`, and `32`.
- Callee homes remain published for `%p.B`, `%p.fdB`, `%p.b`, `%p.C`, and
  `%p.fdC` as stack-slot homes at offsets `32`, `40`, `48`, `56`, and `44`.
- The representative object probe still fails before full emission at the
  existing non-goal
  `unsupported_stack_frame: RV64 object route does not support non-GPR prepared callee-saved register save slots (fpr:fs1)`.

Focused equivalent facts:

- Focused source and artifacts:
  `build/agent_state/512_step6_rv64_consumption/focused_stack_param/`.
- The focused ordinary-C `take9` function publishes `%p.a8` as
  `kind=stack_slot slot_id=5 offset=40`.
- The focused object probe succeeds (`focused_rc=0`) and produces
  `stack_param.o`, proving the RV64 consumer advances past the old
  `unsupported_param_home` family when the prepared stack-passed formal home is
  coherent and no unrelated stack-frame/instruction blocker intervenes.
- The synthetic `backend_riscv_object_emission` coverage also proves an I64
  and F64 stack-passed formal home build, while malformed offsets, frame slots,
  source kinds, missing `passed_on_stack`, memory class, and F128 remain
  fail-closed.

## Suggested Next

Step 7 should probe `20010518-1.c`, `pr27073.c`, and `pr69447.c`, then
classify each as covered by the new stack-parameter contract, blocked by a
narrower producer gap, or intentionally fail-closed for an out-of-scope ABI
feature.

## Watchouts

- `20001017-1.c` remains blocked by the existing `fpr:fs1` callee-saved
  stack-frame diagnostic, not by the stack-passed parameter-home consumer.
- Step 7 should not treat the no-body focused `take9` source as named-case
  proof for the whole bucket; it only isolates the Step 6 RV64 consumer gate.
- Keep any remaining varargs, F128, memory aggregate, dynamic stack, or
  unrelated RV64 instruction-lowering gaps out of this source idea unless the
  supervisor/plan-owner creates a separate follow-up.

## Proof

- `cmake --build build --target c4cll` passed.
- `cmake --build build --target backend_riscv_object_emission_test` passed to
  rebuild the touched C++ test binary before CTest.
- Representative dump/object probe artifacts were captured under
  `build/agent_state/512_step6_rv64_consumption/20001017-1/`; the object probe
  returns rc 2 at the known `fpr:fs1` blocker.
- Focused equivalent dump/object probe artifacts were captured under
  `build/agent_state/512_step6_rv64_consumption/focused_stack_param/`; the
  object probe returns rc 0.
- `ctest --test-dir build -j --output-on-failure -R 'stack_passed_parameter_home|prepare_frame_stack_call_contract|prealloc_formal_publications|backend_riscv_object_emission|riscv64_byval|rv64_runtime_riscv64_byval|riscv64_aggregate' | tee test_after.log`
  passed, 22/22. `test_after.log` is the canonical proof log for this packet.
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`
  passed.
