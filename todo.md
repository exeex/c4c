Status: Active
Source Idea Path: ideas/open/319_aarch64_hfa_aggregate_argument_runtime.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Earliest Argument Corruption

# Current Packet

## Just Finished

Step 1 - Localize Earliest Argument Corruption complete.

The earliest corrupted `00204.c` `Arguments:` value is the first HFA float
argument call:

- Source fact: `arg()` calls `fa_hfa11(hfa11)`, and the expected output line
  immediately after the 17 string aggregate argument lines is `11.1`.
- Observed baseline fact: the same line prints `0.0`; later HFA float, double,
  and long-double argument lines then print `0.0`, huge floating values, and
  `-nan` before the process segfaults.
- Generated BIR fact: with `--target aarch64-unknown-linux-gnu --dump-bir
  --mir-focus-function arg`, the call is still lowered as
  `bir.call void fa_hfa11(ptr byval(size=4, align=4) %t19)`, after loading
  `hfa11.a` into `%t19.0`.
- Generated assembly fact: `arg` loads `hfa11.a` into scratch `s13`, stores it
  to a local stack slot, then branches to `fa_hfa11`; `fa_hfa11` reads scratch
  `s13`/its local byval pointer path instead of receiving the value through an
  AAPCS64 FP argument register such as `s0`.

Likely owning surfaces are the fixed aggregate/HFA ABI lowering path before
machine codegen:

- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: signature parameter rendering
  and `populate_signature_type_refs` currently only mark AMD64 fixed
  aggregates as byval, while AArch64 fixed HFA params do not become structured
  FP-register ABI params.
- `src/codegen/lir/hir_to_lir/call/args.cpp`: AArch64 HFA packing exists for
  variadic aggregates, but fixed aggregate arguments fall through the normal
  fixed aggregate/byval path and reach BIR as pointer byval.
- `src/backend/bir/lir_to_bir/call_abi.cpp`: structured signature and direct
  call lowering consume the byval aggregate metadata and produce BIR
  `ptr byval` params/args instead of HFA scalar FP ABI lanes.
- Downstream consumers (`src/backend/prealloc/legalize.cpp`,
  `src/backend/prealloc/regalloc.cpp`, `src/backend/prealloc/call_plans.cpp`,
  and `src/backend/mir/aarch64/codegen/calls.cpp`) appear to be acting on that
  upstream byval ABI decision rather than independently classifying the fixed
  HFA.

Representative repair coverage should include a local AArch64 ABI/backend test
for fixed HFA arguments, starting with single-float HFA (`struct { float a; }`)
and extending enough to cover adjacent fixed HFA lanes (`hfa12`/`hfa14`) without
special-casing `00204.c`. Existing nearby guard tests are
`backend_aarch64_instruction_dispatch`, `backend_aarch64_target_instruction_records`,
and `backend_aarch64_machine_printer`, but there is a current gap for fixed
AAPCS64 HFA argument classification/lowering from source signature through
prepared call records.

## Suggested Next

Execute Step 2 from `plan.md`: repair fixed AArch64 HFA aggregate argument
classification/lowering so same-module calls pass fixed HFA members through
the AAPCS64 FP argument ABI instead of the generic pointer-byval aggregate
path.

## Watchouts

- There is a separate-looking global data emission symptom in the generated
  assembly (`hfa11:` and related HFA globals still contain the deferred data
  comment), but the first ABI fact to repair for idea 319 is that caller/callee
  lowering for fixed HFAs uses `ptr byval` and scratch/local storage instead of
  FP argument registers. Do not mix global initializer emission into this
  packet unless the ABI path is first corrected and the remaining first bad
  fact proves globals are the blocker.
- Preserve prior-owner guardrails for ideas 314, 315, 317, and 318.
- Do not reopen scalar ALU immediate materialization unless generated-code
  evidence shows an illegal scalar ALU immediate still reaches executable
  code.
- Do not special-case `00204.c`, one output line, one function name, one
  register, or one stack slot.

## Proof

Focused proof command for this localization packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build succeeded; 11 tests ran; 10 passed. The only failing test is the
expected baseline `c_testsuite_aarch64_backend_src_00204_c`, still failing at
runtime with the localized early `Arguments:` corruption (`11.1` expected,
`0.0` observed first) plus later corrupted HFA output and segfault. Proof log:
`test_after.log`.
