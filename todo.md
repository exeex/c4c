Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the localized fixed mixed-HFA `fa3` residual for the `hfa23`
double lanes. Prepared BIR already had the correct `hfa23` global-symbol
accesses and stack-backed FPR storage; the missing owner was AArch64 MIR load
stack publication, which only selected GP scratch registers for stack-published
scalar load results.

The stack-publication load path now uses scalar storage register views and
selects reserved FP/SIMD scratch registers for `F32`/`F64` stack-backed load
results. Generated `arg` code before `bl fa3` now materializes
`hfa23`, `hfa23+8`, and `hfa23+16` through `d16` and publishes those values to
the prepared frame slots before the existing copies load `d4`, `d5`, and `d6`.
This is a general scalar floating stack-publication repair, not a named
`00204.c`, `fa3`, or `hfa23` special case.

Focused backend coverage now pins the repaired owner. The
`backend_aarch64_instruction_dispatch` bucket includes a direct F64 global load
whose result home/storage is a prepared frame slot, and verifies lowering emits
`ldr d16, [x9]` followed by `str d16, [sp, #64]`.

`00204.c` is still not green, but the first bad fact moved. The former `fa3`
line now prints `14.1 14.4 23.1 23.3 32.1 32.2`; the next bad line is the
following fixed mixed call `fa4(s1, hfa14, s2, hfa24, s3, hfa34)`, which prints
garbled small-aggregate text and wrong HFA lanes before the later return-value
corruption and segmentation fault.

## Suggested Next

Continue Step 2 from the new first bad fact: localize the fixed mixed
small-aggregate/HFA `fa4` boundary, especially the ownership of interleaved
small struct, float HFA, double HFA, and binary128 HFA caller publication and
callee consumption. Keep this separate from the repaired scalar floating
stack-publication path unless generated-code evidence points back to it.

## Watchouts

The `fa3` caller now has generated-code evidence for the formerly missing
`hfa23` loads: `adrp/add` for `hfa23`, `hfa23+8`, and `hfa23+16`, each followed
by `ldr d16, [x9]` and a stack publication before the `d4..d6` argument loads.
Do not unwind the earlier HFA group-spill or F128 stack handoff repairs without
fresh regression evidence.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded;
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`
passed, including the new focused F64 stack-publication coverage.
`c_testsuite_aarch64_backend_src_00204_c` still failed with `RUNTIME_NONZERO`
/ segmentation fault. The fresh first bad output has moved past `fa3` to the
following `fa4` fixed mixed-aggregate/HFA line, followed by corrupted
return-value output. `test_after.log` is the fresh proof log for this repair
and remaining blocker.
