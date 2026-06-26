Status: Active
Source Idea Path: ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Admit Scalar Compare Trunc Lowering

# Current Packet

## Just Finished

Completed Plan Step 2 by admitting the first RV64 object-route scalar
compare/trunc materialization path:

- `Sge` compare result materialization is now gated on prepared metadata for a
  named `i32` compare result feeding exactly one `i32 -> i16` trunc.
- The admitted compare shape requires an `i32` GPR lhs, a signed-12-bit
  materializable RHS immediate, a GPR compare-result home, and a supported
  trunc destination home in either a GPR or an `i16` stack slot.
- Stack-slot value publication now loads stack homes using the value's scalar
  width, so the audited `i16` trunc result can publish through a prepared
  stack slot into the return register path.
- Focused tests prove the emitted `slti`/`xori` compare materialization,
  `i16` trunc stack publication, and fail-closed neighbors for unsupported
  predicate, compare width, trunc width, and compare-result home.

## Suggested Next

Execute Plan Step 3 by rerunning the `tests/c/external/gcc_torture/src/20000217-1.c`
representative through the RV64 gcc torture backend runner with the temporary
allowlist, then record whether it passes, advances, or exposes an out-of-scope
owner.

## Watchouts

- Do not reopen frame-slot address call-argument materialization, pointer-value
  local memory, or frame-slot payload-value call arguments; ideas 372, 368, and
  373 already own those routes.
- Do not absorb non-register parameter homes, aggregate `va_arg`, byval homes,
  terminators, CFG reconstruction, or source-shape shortcuts.
- Progress must come from prepared compare/trunc metadata, not testcase names,
  hard-coded constants, value ids, frame slots, or registers.
- The admitted RHS immediate must fit the object-route `slti` immediate field;
  wider immediate materialization remains fail-closed.
- The source path named in the packet as `src/20000217-1.c` exists in this
  checkout as `tests/c/external/gcc_torture/src/20000217-1.c`.

## Proof

Delegated proof passed:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Proof log: `test_after.log`.
