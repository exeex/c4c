Status: Active
Source Idea Path: ideas/open/04_aarch64_prior_preservation_baseline_drift.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Validate Regression Family

# Current Packet

## Just Finished

Step 3 validated the remaining AArch64 c_testsuite family after `12d68c4d6`
and repaired the next same-family call-boundary source-selection hole:
explicit local-frame address selections may now consume the matched
callee-saved prior preservation home when the selected frame facts are not
available at lowering time. This moved `00173`, `00179`, `00186`, and `00187`
to green in the delegated subset.

## Suggested Next

Supervisor should treat the remaining failures as a separate initiative or
delegate a new packet for the next classified family, starting with either
`00181`/`00216` runtime crashes or `00204` AArch64 aggregate/HFA/stdarg ABI
mismatch.

## Watchouts

The remaining red cases are not clearly part of the same
prior-preservation/call-boundary source-selection issue: `00181` is the Tower
of Hanoi recursion/global-array case, `00216` is aggregate initializer and
compound literal layout, and `00204` is AArch64 aggregate/HFA/stdarg ABI
behavior. Do not weaken expectations or fold those into this source-selection
repair without a new route.

## Proof

(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_(00173|00179|00181|00186|00187|00216|00204)_c)$') > test_after.log 2>&1

Failed for the three classified non-source-selection cases: `00181`, `00216`,
and `00204`. `backend_aarch64_instruction_dispatch`, `00173`, `00179`,
`00186`, and `00187` passed. Proof log: `test_after.log`.
