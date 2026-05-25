Status: Active
Source Idea Path: ideas/open/15_aarch64_aggregate_hfa_stdarg_abi.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The Narrow ABI Owner

# Current Packet

## Just Finished

Step 2/3 added focused dispatch coverage for both sides of the small non-HFA
byval register-lane rule. The one-byte register-home case requires
`ldrb w0, [sp, #128]` and rejects both the aggregate home register and the
preservation spill. A new two-byte stack-home reduced test requires the two
payload bytes to be packed from populated local slots with `ldrb`/`orr` instead
of reading the aggregate home slot, and now includes an explicit selected
byval source pointing at the aggregate home to match the real `fa_s2` hazard.

Step 3 repaired the owned AArch64 call-boundary lowering so discovered payload
stores win over aggregate home or preservation sources for small byval
register-lane publication. The real `00204` path now emits `ldrb w0,
[sp, #928]` for `fa_s1` and packs `fa_s2` from `[sp, #929]` plus `[sp, #930]`.
The same payload-store source rule also feeds stack-slot byval lane
publication, so later `fa1`/`fa2` overflow aggregate arguments are copied from
populated payload slots into the outgoing stack area.

## Suggested Next

Supervisor review/commit for the completed Step 2/3 slice.

## Watchouts

The repair is intentionally semantic rather than testcase-shaped: source-name
variants, direct local-slot payload lookup, selected-source mismatch handling,
and omitted stack-slot explicit moves are all generic call-boundary rules. Do
not reintroduce aggregate home or prior-preservation fallback when payload
stores are available.

## Proof

Ran:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$') > test_after.log 2>&1`

Result: passed. Build succeeded; `backend_aarch64_instruction_dispatch` passed,
including the new one-byte register-home and two-byte stack-home reduced
coverage; `c_testsuite_aarch64_backend_src_00204_c` passed. Proof log:
`test_after.log`.
