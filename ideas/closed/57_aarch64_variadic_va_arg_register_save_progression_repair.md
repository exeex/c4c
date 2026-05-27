# AArch64 Variadic Va Arg Register-Save Progression Repair

## Goal

Repair aggregate `va_arg` register-save consumption in
`src/backend/mir/aarch64/codegen/variadic.cpp` so AArch64 `va_list`
register-save source addresses use the old negative offset and only then
publish the advanced `gp_offset` or `fp_offset` progression value.

## Why This Exists

The focused `00204` route no longer points at caller-side call argument
publication. Prepared call facts publish the direct variadic by-value aggregate
lanes correctly, and the callee save area receives the incoming variadic GPR
registers. The remaining mismatch is in callee-side aggregate `va_arg`
lowering: the register-save path advances the progression field before
forming the source address, so a 7-byte aggregate reads the next GPR save slot
instead of the slot selected by the old offset.

## In Scope

- Inspect aggregate `va_arg` register-save lowering in
  `src/backend/mir/aarch64/codegen/variadic.cpp`.
- Preserve the AAPCS64 rule that register-save consumption addresses are based
  on the current negative offset, while the `va_list` progression field stores
  current offset plus the prepared stride after consumption.
- Cover both `gp_offset` and `fp_offset` register-save paths if they share the
  same progression ordering bug.
- Preserve overflow-area consumption, stack-published field stores, and HFA or
  lane-home behavior unless the same pre-increment source-address rule directly
  applies.
- Validate with the focused eight-test subset that has been tracking `00204`
  and nearby variadic/call-authority probes.

## Out Of Scope

- Do not change caller-side call argument publication in `calls.cpp` or
  `call_plans.cpp`.
- Do not reopen edge terminator consumer preservation, scalar cast source
  consumption, memory field publication, or dispatch edge-copy routing.
- Do not change AAPCS64 register-save area constants, register numbering,
  overflow branch selection, direct-call spelling, or stack cleanup rules
  except where required to use the old progression value for source addressing.
- Do not fix this by matching `00204`, `myprintf`, format strings, temporary
  names, or literal string contents.
- Do not weaken tests, mark supported paths unsupported, or rewrite expected
  output as proof.

## Acceptance Criteria

- Aggregate `va_arg` register-save lowering computes the source address from
  the pre-increment `gp_offset` or `fp_offset` value.
- The corresponding progression field is still advanced by the prepared stride
  after the source address for the current aggregate has been selected.
- Existing overflow-area and lane-home behavior remains intact.
- The focused proof command passes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
- If the focused proof remains red, `todo.md` records the next precise
  variadic register-save owner rather than expanding back into calls or memory
  routes without evidence.

## Closure Note

Closed after the focused variadic register-save close gate passed with matching
canonical logs. `test_before.log` was regenerated from temporary worktree
commit `fc2463b69` using the focused eight-test command and showed `7/8`,
failing only `c_testsuite_aarch64_backend_src_00204_c`. Current
`test_after.log` used the same focused command and passed `8/8`. The
regression guard reported the `00204` failure resolved with no new failures.

The rejected full-suite `test_baseline.new.log` was not used for closure.

## Reviewer Reject Signals

- Reject changes that key behavior to `00204`, `myprintf`, `%7s`, `%9s`,
  temporary names, string literal contents, or exact emitted stack offsets.
- Reject caller-side call publication rewrites claimed as progress for this
  idea unless the executor first proves the callee register-save ordering
  diagnosis is wrong.
- Reject expectation downgrades, unsupported-test rewrites, helper renames, or
  classification-only changes claimed as capability progress.
- Reject broad variadic rewrites that alter overflow-area selection, register
  save area constants, HFA lane homes, or unrelated scalar `va_arg` behavior
  without a direct proof that they share the same progression-ordering bug.
- Reject a patch that still forms aggregate register-save source addresses
  from the advanced `gp_offset` or `fp_offset` while hiding the old failure mode
  behind a new abstraction name.
