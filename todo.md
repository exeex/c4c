Status: Active
Source Idea Path: ideas/open/332_aarch64_movi_zero_extension_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The MOVI Sign-Extension Owner

# Current Packet

## Just Finished

Lifecycle switched from parked idea 326 after commit `b5975e0cd`. The HFA
long-double, double, and float sections are repaired; the remaining
representative failure is later in MOVI, with expected `abcd0000` and actual
`ffffffffabcd0000`. Nearby MOVI values also show sign-extension where expected
output uses zero-extension, for example expected `aaaaaaaa` and actual
`ffffffffaaaaaaaa`.

## Suggested Next

Start Step 1 by mapping the MOVI output mismatch back to source expressions,
prepared BIR values, generated AArch64 instructions, and the observing output
call. Localize the first point where the high bits become sign-filled.

## Watchouts

Do not reopen HFA overflow assignment, scalar-FP symbol-load placement, byval
aggregate lane publication, stdarg cursor, fixed-formal entry publication, or
local/value-home publication without fresh generated-code evidence. Do not
special-case `00204.c`, the MOVI block, `abcd0000`, `aaaaaaaa`, one register,
one stack offset, or one emitted instruction sequence. Leave
`review/326_stdarg_byval_route_review.md` untouched.

## Proof

Lifecycle-only switch. Prior evidence from the parked 326 route: focused proof
passed the prepared `00204.c` handoff dump and byval payload helpers, while
`c_testsuite_aarch64_backend_src_00204_c` failed at the advanced MOVI
sign-extension mismatch. Supervisor broader validation passed
`ctest --test-dir build -j --output-on-failure -R '^backend_'` with 140/140
tests passing.
