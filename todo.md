Status: Active
Source Idea Path: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Address-Valued Call Argument Publication

# Current Packet

## Just Finished

Step 3 repaired address-valued call-argument publication for idea 355.
Frame-slot call arguments that have prepared address-materialization facts now
prefer the address source over a prior preserved stack reload, so an
address-exposed local passed to a pointer parameter materializes the frame
address instead of loading an unpublished pointer-sized home.

Stack-homed GOT-required global loads now publish their prepared stack home at
the load site. Later call-boundary moves reload that home rather than trusting a
scratch register that may be reused by another global load before the call.

Focused backend coverage now proves both Step 3 representatives: `00170` and
`00189` pass, while the Step 2 representatives `00020` and `00103` remain
passing.

## Suggested Next

Proceed to Step 4 with the supervisor-selected adjacent pointer subset,
checking whether `00005`, `00173`, and `00181` remain in scope or need a
separate lifecycle owner.

## Watchouts

- Do not edit expectations, unsupported classifications, runners, timeout
  policy, CTest registration, or proof-log behavior.
- Do not special-case c-testsuite filenames, source function names, stack
  offsets, symbol names, or emitted instruction neighborhoods.
- `00170` and `00189` now pass through semantic call-boundary publication; do
  not regress them by reintroducing preserved stack reloads for address
  arguments or late publication from reused scratch registers.
- Keep scalar compare/select, floating variadic scalar, composite ABI,
  dynamic-stack/goto, and aggregate initializer residuals out of this owner
  unless fresh first-bad-fact evidence justifies a lifecycle split.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00020_c|c_testsuite_aarch64_backend_src_00103_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log
```

Result: passed, 8/8. The backend unit/CLI contracts passed, and
`c_testsuite_aarch64_backend_src_00020_c`,
`c_testsuite_aarch64_backend_src_00103_c`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c` all passed.

Proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
