Status: Active
Source Idea Path: ideas/open/354_aarch64_external_call_symbol_home_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Publication Handoff

# Current Packet

## Just Finished

Steps 2 and 3 added focused coverage for a direct external call whose
string-symbol arguments have preserved stack homes, then repaired the dispatch
handoff that reloaded those stack homes after address materialization.

The fix keeps the shared AArch64 call path from emitting a preserved-home reload
when the same address value has already been materialized into the same
call-argument register. The focused coverage proves `.fmt` and `.payload`
string-symbol arguments are materialized into `x0` and `x1` and are not
overwritten by `ldr` from their preserved stack homes before `bl printf`.

`c_testsuite_aarch64_backend_src_00187_c` now passes the delegated proof. A
generated-assembly sanity check for the second `fopen` shows the old stack-home
reloads after `.str0`/`.str3` materialization are gone before `bl fopen`.

## Suggested Next

Ask the supervisor to run any broader guard it wants for Step 4 and decide
whether idea 354 is ready for lifecycle closure.

## Watchouts

- The repair intentionally avoids reloading an unpublished preserved stack home
  only when an address materialization for the same value already owns the same
  call-argument register.
- The generated assembly still contains a duplicate `.str0` materialization
  before the second `fopen`; it is redundant but no longer clobbers the ABI
  arguments with stale stack loads.
- Preserve the completed `00130`, `00176`, and `00195` guardrails from idea
  348 during supervisor-side validation.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00187_c)$' | tee test_after.log`

Result: passed, 5/5. Proof log: `test_after.log`.

Additional generated-assembly sanity check:

`./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00187.c -o /tmp/c4c_00187_aarch64_after.s`

The second `fopen` path now materializes `.str0`/`.str3` into call-argument
registers and reaches `bl fopen` without the prior `ldr x0, [sp, #32]` or
`ldr x1, [sp, #40]` overwrite.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
