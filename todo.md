Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Current HFA/Floating Residual

# Current Packet

## Just Finished

Step 1 localized the current `00204.c` variadic HFA/floating residual without
implementation edits. The first concrete `myprintf` HFA owner break is the
`%hfa12` branch in `build/c_testsuite_aarch64_backend/src/00204.c.s`: source
`tests/c/external/c-testsuite/src/00204.c:263` expects `printf("%.1f,%.1f",
x.a, x.b)` for payload `{12.1f, 12.2f}`. Generated assembly selects two
float HFA lanes from the AAPCS64 FP register-save-area path into `sp + 720`
and `sp + 724` at assembly lines 6340-6358, then the observing `printf`
publishes from unrelated homes `sp + 556` and `sp + 560` at lines 6368-6379.
Those selected lanes are never copied/published into the homes consumed by
the following call, so classification is HFA aggregate `va_arg` destination
payload publication / temporary-object home binding, not source progression.

The guardrail evidence still keeps the closed non-HFA string route closed:
the preceding `%7s` and `%9s` branches copy selected bytes into the printf
buffers before observing calls, e.g. `%7s` populates `sp + 593..599`, copies
to `sp + 8..14`, and calls `printf("%.7s", sp + 8)` at assembly lines
6140-6185.

Long-double HFA branches show the adjacent same-owner publication gap in a
more severe form: `%hfa31` / `%hfa32` select 16-byte/32-byte payload chunks
into stack temporaries such as `sp + 824`, `sp + 832`, `sp + 840`, `sp + 848`,
but call `.str44` (`"%.1Lf,%.1Lf"`) at assembly lines 6819-6821 and
6890-6892 with no `x1`/stack argument publication for either selected
`long double` value.

Likely owning code surfaces are:
`src/backend/prealloc/variadic_entry_plans.cpp`
(`infer_aapcs64_hfa_va_arg_shape`,
`make_aapcs64_aggregate_va_arg_access_plan`) for recognizing HFA aggregate
`va_arg` lanes and destination homes, and
`src/backend/mir/aarch64/codegen/variadic.cpp`
(`print_aggregate_va_arg_lowering_lines`) for copying register-save-area HFA
lanes into the selected destination payload. If the repair needs to publish
that selected payload to the following call, inspect the AArch64 call
publication surfaces in `src/backend/mir/aarch64/codegen/calls.cpp` and
`src/backend/mir/aarch64/codegen/dispatch.cpp` only after confirming the
payload home in prepared/BIR facts.

## Suggested Next

Start Step 2 by repairing AAPCS64 variadic aggregate HFA `va_arg` publication
generally: after `llvm.va_arg.aggregate` selects HFA lanes from the FP
register-save-area or overflow path, the selected destination payload home
must be the same object/lane home later consumed by scalar field loads and
the observing `printf` call. Cover float/double HFA lanes and the long-double
path that currently reaches `.str44` without argument publication.

## Watchouts

Do not reopen the closed non-HFA aggregate string materialization route unless
fresh generated-code evidence shows selected `%7s` / `%9s` bytes are again
missing before their observing `printf` calls.

Preserve the prior-owner guardrails named in `plan.md`, especially idea 330
non-HFA aggregate `va_arg` copies and idea 329 post-`va_arg` call publication.

Representative existing tests for the repair surface:
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`,
`backend_lir_to_bir_notes`,
`backend_prepared_printer`, and
`c_testsuite_aarch64_backend_src_00204_c`. The smallest focused proof command
after a repair should start with:
`ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

## Proof

Ran the delegated proof command after localization:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$'`.

Result: build was up to date; the focused CTest still fails with
`RUNTIME_NONZERO` / segmentation fault. Current output in `test_after.log`
shows malformed HFA/floating output remains, consistent with the generated
artifact evidence above. This proof is sufficient for localization but not an
acceptance proof for Step 2 implementation.
