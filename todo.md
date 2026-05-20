Status: Active
Source Idea Path: ideas/open/337_aarch64_callee_saved_scalar_home_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Missing Callee-Saved Preservation

# Current Packet

## Just Finished

Lifecycle switched from closed idea 336 to residual idea 337. No executor
packet has run for this plan yet.

## Suggested Next

Start Step 1 by localizing where recursive `factorial` in
`c_testsuite_aarch64_backend_src_00168_c` keeps caller `n` in `w19` across
`bl factorial` while the frame preservation set omits `x19`.

## Watchouts

Do not repair this through return-publication suppression. Idea 336 closed the
stale terminal return overwrite; this plan owns live callee-saved scalar-home
preservation across calls.

## Proof

No executor proof yet for idea 337. The close gate for idea 336 passed with
matching `test_before.log` and `test_after.log` over the focused
return-publication scope.
