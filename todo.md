Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the prepared parameter-home owner for AArch64 fixed formals
with interleaved GPR/FPR pressure. Prealloc now computes fixed-formal ABI
register indexes per AArch64 register bank before publishing parameter homes,
so the `fa4` HFA lane homes are `hfa14 -> s0..s3` and `hfa24 -> d4..d7`
instead of flat-index shifted homes like `s1..s4` and partial `d6/d7`.

Prealloc also uses prepared addressing metadata to seed stack-passed F128
formal lanes into their local frame slots. The generated `fa4` entry now loads
incoming `hfa34` `q` lanes from caller stack offsets `sp+320/336/352/368` and
stores them into local homes at `sp+16/32/48/64`, avoiding the prior overlap
with small byval aggregate homes at `sp+0..5`.

Focused backend coverage was updated in
`backend_aarch64_instruction_dispatch`: a semantic prepared-route fixture now
drives the interleaved byval/HFA/F128 shape through prealloc and asserts the
published `s0..s3`, `d4..d7`, and distinct aligned stack-passed F128 homes.
The existing hand-built dispatch fixture still verifies the resulting entry
publication sequence.

`00204.c` is still not green, but the targeted `fa4` line now matches the
expected output exactly: `0 14.1 14.4 12 24.1 24.4 345 34.1 34.4`. The first
observed output mismatch in the full test is now earlier/later unrelated
aggregate-return fallout: after `WXYZ0123456789ab`, the expected
`cdefghijklmnopqrs` line is blank, and return-value output later corrupts
starting at `fr_s4`/HFA return paths. This slice is real progress because the
generated-code evidence no longer contains shifted `fa4` FPR homes, missing
`hfa24` lane homes, or stack-passed F128 homes copied into the low byval area.

## Suggested Next

Continue Step 2 on the next remaining `00204.c` owner outside `fa4` fixed
formal homes. The fresh failing output points at aggregate and HFA return
materialization: first the `fa_s17`/large-aggregate line is blank, then return
values become corrupt from `fr_s4` and HFA returns.

## Watchouts

The generated `arg -> fa4` caller remains coherent:
`ldrb w0` for `s1`, `s0..s3` for `hfa14`, `ldrh w1` for `s2`, `d4..d7` for
`hfa24`, `x2` for `s3`, and four outgoing stack `q` stores for `hfa34`.

The generated `fa4` callee now has no `fmov s1, s0` / `fmov d6, d4` style
entry shuffles. It directly stores `s0..s3`, `d4..d7`, and stack-loaded
`q16` lanes into prepared local homes. Remaining `00204.c` failure should not
be chased by changing this fixed-formal owner unless new evidence points back
to it.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded;
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and
`backend_aarch64_instruction_dispatch` passed, including the new semantic
mixed GPR/HFA/F128 prepared-home coverage. `c_testsuite_aarch64_backend_src_00204_c`
still failed with `RUNTIME_NONZERO` / segmentation fault after printing the
now-correct fixed mixed `fa4` line. `test_after.log` is the fresh proof log
for this packet and remaining blocker.
