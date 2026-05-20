Status: Active
Source Idea Path: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Variadic Aggregate Va Arg Post-Consumption Call Setup

# Current Packet

## Just Finished

Lifecycle switched from parked idea 328 to the adjacent variadic aggregate
post-`va_arg` call-setup idea. No implementation packet has run for this new
plan yet.

## Suggested Next

Execute Step 1. Start from generated `myprintf` for `00204.c`, especially the
`%7s` and `%9s` branches that should call `printf("%.7s", t7.x)` and
`printf("%.9s", t9.x)` but currently reach `printf` with `x0 == 0x1`.

## Watchouts

Do not reopen idea 328 unless fresh generated code moves the first bad fact
back to fixed byval aggregate call-argument lane publication. The current
residual is non-HFA aggregate `va_arg` plus following ordinary call setup; the
parked HFA/floating residual remains out of scope until fresh evidence reaches
that path.

## Proof

Lifecycle-only activation. Existing `test_before.log` and `test_after.log`
cover the same 12-test focused scope and have no new failures, but strict
close guard for idea 328 was not accepted because the pass count did not
increase.
