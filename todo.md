Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Stack-Passed Parameter Homes

# Current Packet

## Just Finished

Step 3 of `plan.md` published producer-side RV64 stack-passed parameter
homes without changing RV64 object-emission inference.

Implementation boundary:

- `call_arg_destination_stack_offset_bytes` now admits RV64 through the shared
  caller ABI helper only for ordinary fixed stack-passed arguments with
  explicit call-arg ABI facts. RV64 byval, sret, and memory-class aggregate
  forms do not receive destination stack offsets from this path.
- `classify_prepared_value_home` now has a narrow RV64 fixed-formal stack-home
  branch. It only publishes `PreparedValueHomeKind::StackSlot` for
  `PreparedValueKind::Parameter` formals with explicit producer ABI metadata:
  `param.abi.has_value()` and `param.abi->passed_on_stack` are required.
- The previous sparse-ABI/index fallback was removed. The route no longer
  derives stack-passed formal homes from parameter index, parameter name, or a
  late non-Parameter name match.
- The previous broad RV64 caller offset admission was narrowed after byval
  regression review. Caller offsets are limited to non-byval, non-sret,
  non-memory-class stack arguments with explicit `passed_on_stack` ABI.
- RV64 object emission, unsupported markers, expectation files, allowlists,
  runtime comparison, and scan accounting were not changed.

Representative before/after facts for `tests/c/external/gcc_torture/src/20001017-1.c`:

- Before: caller stack ABI bindings for indexes 8-12 had
  `destination_storage=stack_slot` but no `stack_offset`.
- After byval-safe tightening, the representative still has stack-slot ABI
  bindings for indexes 8-12 without `stack_offset`, because this gcc_torture
  route does not expose suitable explicit call-arg stack ABI authority.
- Before: callee homes for `%p.B`, `%p.fdB`, `%p.b`, `%p.C`, and `%p.fdC`
  were all `kind=none`.
- After tightening authority, those representative callee homes remain
  `kind=none` because the route lacks explicit formal `passed_on_stack` ABI
  metadata. The withdrawn `%p.B`/`%p.fdB`/`%p.fdC` home improvement depended
  on index/name inference and was not accepted as Step 3 progress.
- Focused synthetic coverage now proves the legitimate callee path: an
  explicit-ABI RV64 fixed formal with `passed_on_stack=true` publishes a
  stack-slot home with slot, offset, size, and alignment facts.
- Focused synthetic coverage also proves the legitimate caller path:
  explicit ordinary RV64 stack call arguments publish stack offsets. Existing
  RV64 byval dump, codegen, and runtime tests remain preserved.

## Suggested Next

Execute Step 4 by consuming the newly published producer records through the
RV64 route:

- Keep RV64 reading prepared call/formal records directly; do not reconstruct
  stack offsets from argument index, source shape, or parameter spelling.
- Confirm whether the current `fpr:fs1` save-slot rejection must remain a
  non-goal blocker or be handled by a separate active-plan step before the old
  parameter-home rejection can be observed again in object codegen.
- Preserve the fail-closed behavior for stack-passed formals without explicit
  producer ABI and coherent home facts, demonstrated here by
  `%p.B`, `%p.fdB`, `%p.b`, `%p.C`, and `%p.fdC` remaining `kind=none`.
- Keep byval, sret, and aggregate/memory ABI paths out of the ordinary stack
  argument offset publication route.

## Watchouts

- The callee helper deliberately requires explicit `param.abi->passed_on_stack`
  plus a unique matching `regalloc.spill_slot` object and frame slot. Missing
  ABI, ambiguous, duplicate, byval, sret, varargs, F128, dynamic, or aggregate
  forms should remain fail-closed.
- The caller helper deliberately refuses RV64 destination stack offsets for
  byval, sret, and memory-class aggregate arguments. Those paths remain owned
  by existing byval/aggregate transport logic.
- `20001017-1.c` object codegen still fails before parameter-home consumption
  at `unsupported_stack_frame ... fpr:fs1`; this slice does not claim object
  advancement.
- The new dump test proves the representative row does not infer stack offsets
  or stack homes without explicit authority. Synthetic C++ contract coverage
  proves explicit ordinary caller/formal producer facts.

## Proof

- `cmake --build build --target c4cll` passed.
- `ctest --test-dir build -j --output-on-failure -R 'stack_passed_parameter_home|prepare_frame_stack_call_contract|prealloc_formal_publications|backend_riscv_object_emission|riscv64_byval|rv64_runtime_riscv64_byval' | tee test_after.log`
  passed, 15/15. `test_after.log` is the canonical proof log for this packet.
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c`
  was captured before, after the rejected inference fallback, and after the
  tightened authority boundary under
  `build/agent_state/512_step3_stack_parameter_publication/20001017-1/`.
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20001017-1.c -o build/agent_state/512_step3_stack_parameter_publication/20001017-1/20001017-1.o`
  still fails with rc 2 at the existing `fpr:fs1` unsupported stack-frame
  diagnostic.
- `backend_prepare_frame_stack_call_contract` now includes explicit-ABI
  synthetic coverage for RV64 stack-formal home publication and ordinary
  stack call-argument offset publication.
