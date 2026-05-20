Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Advanced HFA/Aggregate Output Mismatch

# Current Packet

## Just Finished

Lifecycle switched from idea 328 after commit `0a4ef64e5` fixed the byval
payload lane residual and advanced the representative to an HFA/aggregate
output mismatch.

## Suggested Next

Begin Step 1 by localizing the `33.*` / `34.*` mismatch in generated AArch64
artifacts for `00204.c`.

## Watchouts

Do not reopen byval aggregate lane publication unless fresh evidence shows
prepared byval bytes again failing to reach their AAPCS64 call lanes. Leave
`review/326_stdarg_byval_route_review.md` untouched.

## Proof

Lifecycle-only switch. Incoming evidence from idea 328: focused proof passed
the prepared handoff dump and payload helper coverage, failed the
representative only after the fixed byval payloads, and supervisor broader
validation passed `ctest --test-dir build -j --output-on-failure -R
'^backend_'` 140/140.
