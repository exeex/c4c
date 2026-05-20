Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Step 2 now carries generated HFA lane identity through LIR/BIR call metadata,
applies the AArch64 HFA pressure pass to variadic call argument lanes, and
keeps an HFA whole when fewer FP/SIMD argument registers remain than the HFA
lane count requires. Prepared BIR for `00204.c` callsite `stdarg` block
`entry` inst `491` assigns the first `hfa33` to `q0..q2`, the first `hfa34` to
`q3..q6`, and the second `hfa34` wholly to overflow stack at offsets `0`,
`16`, `32`, and `48`; the third `hfa34` starts at stack offset `64`. The
focused dump test asserts both the no-split stack placement and absence of the
old `q7` assignment for arg8.

This packet also aligned caller and callee overflow strides for non-f128 HFA
lanes where payload size is not naturally an 8-byte multiple. F32 HFA lanes
remain packed by lane size inside the object, but the next overflow HFA object
now advances to the same 8-byte aligned stack argument boundary the caller uses
for lane0 placement.

This packet also repaired the remaining F32/F64 HFA payload materialization
owner without a global FPR remap. AArch64 symbol loads for scalar FP values
with an explicit prepared storage placement now keep that storage placement
instead of being retargeted to the printed prepared home name. The representative
HFA double/float bad facts advanced: the generated caller no longer emits
`ldr d21, [x9]` followed by `str d9, ...` for HFA lane global loads, and the
`00204.c` runtime HFA long-double, double, and float sections now match the
expected output.

The focused runtime proof is still red after the HFA repair, but the first bad
fact has advanced outside the HFA overflow contract. Current first mismatch is
in `00204.c` `MOVI`: expected `abcd0000`, actual `ffffffffabcd0000`. Nearby
actual MOVI lines also show sign-extended values where the expected output
uses zero-extended payloads, for example expected `aaaaaaaa`, actual
`ffffffffaaaaaaaa`. This route is still making progress for Step 2 because the
HFA output owner is now repaired; the remaining runtime failure should be
reviewed as the next packet owner before this dirty slice is accepted.

## Suggested Next

Have the supervisor decide whether the advanced MOVI sign-extension mismatch
belongs in this Step 2 slice or needs plan review/a separate packet. Preserve
the completed no-split HFA assignment, HFA overflow stride alignment, and
narrow scalar-FP symbol-load placement consistency repair.

## Watchouts

Do not revert the no-split HFA assignment, HFA overflow stride alignment, or
the scalar-FP symbol-load placement consistency guard to make the later MOVI
failure visible differently. Do not special-case `00204.c`, `.str53`, one HFA
width, one source temp, or one stack offset. The current patch touched generated
LIR call lane identity, BIR/prealloc call ABI plumbing,
`src/backend/prealloc/variadic_entry_plans.cpp`, and
`src/backend/mir/aarch64/codegen/dispatch.cpp`. Leave
`review/326_stdarg_byval_route_review.md` untouched.

## Proof

Built with `cmake --build build --target c4cll -j 2`. Proof command run:
`ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1`.
Result: 3/4 passed. Passing tests:
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
`backend_runtime_byval_helper_payload_8_to_13`, and
`backend_runtime_byval_helper_payload_9_to_14`. Failing test:
`c_testsuite_aarch64_backend_src_00204_c`, at the advanced MOVI
sign-extension mismatch recorded above. `git diff --check` passed. Canonical
proof log: `test_after.log`.

Supervisor broader validation passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed 140/140.
