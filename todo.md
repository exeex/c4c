Status: Active
Source Idea Path: ideas/open/58_aarch64_prepared_authority_regression_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Compare The `c92708627` Boundary

# Current Packet

## Just Finished

Completed Step 2 boundary comparison for `c92708627` using `git show`,
`git diff`, the active focused logs, and the named full-suite baselines.

Boundary behavior:

- `log/baseline_db1581b3833ae7d45fb379623428490cf60a4b36.log` is green.
- `log/baseline_c9270862701eed736e54a77234bd8060ec318e64.log` first shows the
  persistent `backend_aarch64_instruction_dispatch`,
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
  `c_testsuite_aarch64_backend_src_00207_c` failures.
- The `c92708627` code diff is narrow: in
  `src/backend/mir/aarch64/codegen/alu.cpp`, scalar fallback selection and the
  prepared scalar ALU operand override started preferring
  `make_unpublished_load_local_source_operand(...)` before an already emitted
  scalar register. In `src/backend/prealloc/publication_plans.cpp`,
  `find_prepared_same_block_load_local_source_producer(...)` tightened the
  load-access/result match before replacing the producer access by result name.

Likely semantic owner:

- The owner is the AArch64 prepared load-local source consumption surface, not a
  testcase-specific route: `make_unpublished_load_local_source_operand(...)`,
  its `make_prepared_scalar_load_source(...)` memory operand construction, and
  the `lower_scalar_instruction(...)` prepared ALU operand override that chooses
  that memory source over emitted scalar register state.
- The shared prepared lookup owner is
  `prepare::find_prepared_same_block_load_local_source_producer(...)`; Step 3
  should consume or refine that lookup rather than reintroducing broad local
  BIR scans in AArch64 lowering.

Failure relationship:

- Dynamic-stack anchoring is the clearest direct symptom. The focused
  `test_after.log` assembly reloads fixed slots with `[sp, #12]` and `[sp, #8]`
  after `mov sp, x17`; the missing `add w13, w13, #1` follows from consuming a
  prepared frame-slot memory operand without applying the fixed-slot
  frame-pointer base policy. Ordinary memory lowering has
  `apply_frame_pointer_base_policy(...)`, but ALU-produced
  `MemoryOperand`s from `make_prepared_scalar_load_source(...)` do not currently
  carry `uses_frame_pointer_base`.
- The instruction-dispatch failure is related through the same authority shift:
  the failing stack-homed i32 select before a global store now leaves a prepared
  select/publication family unsupported after the store-source path consumes a
  prepared load-local memory source instead of the previously emitted selected
  value. This should be repaired by preserving the semantic publication/operand
  ownership, not by adding a dispatch-test branch.
- `00207` matches the same `c92708627` family first because it times out in a
  VLA/backward-goto and logical/conditional-expression workload after the
  prepared load-local memory-source preference is introduced. Treat it as
  dynamic-stack plus prepared selected-value pressure until Step 3 proof reduces
  the shape.
- `00196` is not owned by this boundary: it remains deferred to the later
  `78730af2f` comparison unless the Step 3 repair also changes its focused
  failure shape.

## Suggested Next

Delegate Step 3 to repair the first boundary family in
`src/backend/mir/aarch64/codegen/alu.cpp`, starting at
`make_unpublished_load_local_source_operand(...)` and the prepared ALU operand
override in `lower_scalar_instruction(...)`.

Concrete target: when a prepared same-block load-local source is converted to a
frame-slot `MemoryOperand`, make that operand safe for its consumer under the
current frame plan, especially by carrying the frame-pointer base requirement
for dynamic-stack fixed slots. Then verify whether the operand preference over
emitted scalar registers still preserves selected-value publication state for
the dispatch/global-store path; if it does not, constrain the override
semantically by consumer/publication state rather than by testcase name.

## Watchouts

- Do not revert the prepared-authority direction by restoring duplicate local
  same-block scans in AArch64 ALU lowering. Use the prepared lookup as the
  semantic owner.
- Do not make `uses_frame_pointer_base` conditional on exact offsets or local
  names. The condition is the function frame plan requiring FP anchoring for
  fixed slots.
- Re-run the focused four-test subset after Step 3. If only dynamic-stack and
  `00207` improve while dispatch remains unsupported, the next owner is the
  selected store/global-store publication path that consumes the ALU-selected
  value, not the test expectation.
- Keep `00196` out of the Step 3 success criteria unless it changes
  incidentally; it is a later-boundary family by the baseline evidence.

## Proof

No fresh test proof was required for this read-only comparison packet.
Evidence used:

```sh
git show --stat --oneline c9270862701eed736e54a77234bd8060ec318e64
git diff c9270862701eed736e54a77234bd8060ec318e64^ c9270862701eed736e54a77234bd8060ec318e64 -- src/backend/mir/aarch64/codegen/alu.cpp src/backend/prealloc/publication_plans.cpp
```

Also inspected `test_before.log`, `test_after.log`, and the baseline logs named
in `plan.md`. Existing canonical proof log remains `test_after.log`.
