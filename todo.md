Status: Active
Source Idea Path: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Minimal Indirect Memory Publication

# Current Packet

## Just Finished

Step 2 repaired the minimal indirect-memory boundary for idea 355:
prepared AArch64 lowering now handles a `LoadLocalInst` whose address is
`base=pointer_value` when the pointer value itself is stack-homed. The new
lowering reloads the stack-homed pointer, performs the final pointee load, and
publishes the prepared result home before downstream consumers observe it.

Focused backend coverage now constructs the `00020` shape directly:
frame-slot pointer load, pointer-value load into a stack home, pointer-value
load through that stack-homed pointer, and a return consumer. The generated
route includes the stack-home publication and final pointee load before the
return move.

The delegated proof advanced the localized boundary: `00020` and adjacent
`00103` both pass.

## Suggested Next

Proceed to Step 3 with a focused call-argument publication packet for one
call-boundary representative, starting with `00170` or `00189` as selected by
the supervisor.

## Watchouts

- Do not edit expectations, unsupported classifications, runners, timeout
  policy, CTest registration, or proof-log behavior.
- Do not special-case c-testsuite filenames, source function names, stack
  offsets, symbol names, or emitted instruction neighborhoods.
- Keep Step 3 distinct from the repaired Step 2 path: `00170` needs
  materialize-address for an address-exposed local passed as a call argument;
  `00189` needs publication of a loaded global pointer into its preserved stack
  home before a later call-argument reload.
- Keep scalar compare/select, floating variadic scalar, composite ABI,
  dynamic-stack/goto, and aggregate initializer residuals out of this owner
  unless fresh first-bad-fact evidence justifies a lifecycle split.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00020_c|c_testsuite_aarch64_backend_src_00103_c)$' | tee test_after.log
```

Result: passed, 6/6. The backend unit/CLI contracts passed, and
`c_testsuite_aarch64_backend_src_00020_c` plus
`c_testsuite_aarch64_backend_src_00103_c` both passed.

Proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
