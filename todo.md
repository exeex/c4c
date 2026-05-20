Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Later Stdarg Field Mismatch

# Current Packet

## Just Finished

Plan step 1 localized the later `stdarg:` mismatch without implementation
edits. The first visible mismatch remains in
`c_testsuite_aarch64_backend_src_00204_c`: expected
`ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI`, observed
`ABCDEFGHI ABCDEFGHI ABCDEFGHI stdarg:` at the first `stdarg` payload line.

The caller-side publication for `stdarg`'s first `myprintf("%9s ...")` call is
not the first divergence. Prepared AArch64 BIR records the expected split:
`arg1` from stack+7752 to `x1/w2[x1,x2]`, `arg2` to `x3/w2[x3,x4]`, `arg3` to
`x5/w2[x5,x6]`, then `arg4`, `arg5`, and `arg6` to overflow stack offsets
`0`, `16`, and `32`. That matches AAPCS64 for six rounded 16-byte `struct s9`
varargs after the format pointer: three complete aggregates in remaining GPRs,
then three complete aggregates on the overflow area.

The exact first generated-code divergence is inside `myprintf`'s `%9s`
`va_arg` transition block. Semantic/prepared BIR for `vaarg.regtry.37` is:
`%t41 = bir.add i32 %t35, 16`; store `%t41` to `ap.__gr_offs`;
`%t42 = bir.sle i32 %t41, 0`; `bir.cond_br i32 %t42, vaarg.reg.38,
vaarg.stack.36`. With initial `__gr_offs = -56`, the first three `%9s`
iterations produce `-40`, `-24`, and `-8`; the fourth computes `+8`, so `%t42`
is false and should branch to `vaarg.stack.36`.

The emitted assembly for the same block materializes the compare and then
clobbers it before the branch:
`cmp w9, #0`; `cset w9, le`; `mov w10, w13`; `mov w9, #16`; `add x10, x10,
x9`; `cbnz w9, .LBB154_27`. Because `w9` is overwritten with `16`, `cbnz` is
always taken to the register path. On the fourth `%9s`, `myprintf` reads from
the register-save area at old offset `-8` instead of switching to the overflow
area where the fourth `s9` was correctly published. The visible effect is three
good `ABCDEFGHI` fields, then traversal continues into the next literal marker
instead of printing the remaining three overflow `s9` fields.

Owner classification: not format traversal, selected aggregate bytes,
destination buffering, or observing call publication. The likely owner is
materialized-bool branch emission/liveness around the generic lowered
non-HFA aggregate `va_arg` cursor progression path: the cursor decision is
correct in BIR, but AArch64 assembly branches on a clobbered condition register.
Likely code surfaces are AArch64 branch/compare materialized-bool printing and
instruction lowering (`src/backend/mir/aarch64/codegen/machine_printer.cpp`,
the branch/compare record path) plus the regalloc/liveness handoff that lets the
dead add/stride temporary reuse the branch condition register before `cbnz`.

## Suggested Next

Repair materialized-bool conditional branch emission so the branch consumes the
actual `%t42` result after any intervening lowered instructions, or eliminate
the intervening dead/clobbering add in `vaarg.regtry.37`. The repair should be
general over materialized bool branch conditions and the AAPCS64 aggregate
`va_arg` register-to-overflow transition, not shaped to `00204.c`, `%9s`, or a
single register/offset.

## Watchouts

Do not reopen fixed byval rounded-slot placement, outgoing stack argument base,
HFA/floating publication, non-HFA aggregate materialization, or post-`va_arg`
call setup without generated-code evidence that the first bad fact moved back
to that owner.

The prepared focused dump already proves caller publication for the first
`myprintf` call is credible. The important repair proof is the fourth `%9s`
transition: after `__gr_offs` advances from `-8` to `+8`, generated assembly
must branch to the overflow path and continue printing all six fields. Avoid a
patch that only rewrites expectations, hard-codes `myprintf`, or special-cases
one `s9` size/offset sequence.

## Proof

Localization probes used:
`/workspaces/c4c/build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function stdarg tests/c/external/c-testsuite/src/00204.c`,
`/workspaces/c4c/build/c4cll --dump-bir --target aarch64-linux-gnu --mir-focus-function myprintf tests/c/external/c-testsuite/src/00204.c`,
`/workspaces/c4c/build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function myprintf --mir-focus-block vaarg.regtry.37 tests/c/external/c-testsuite/src/00204.c`,
and assembly inspection of
`build/c_testsuite_aarch64_backend/src/00204.c.s`.

Executor proof log: `test_after.log` from
`ctest --test-dir build --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$'`.
It still fails at the known runtime mismatch and preserves the expected vs
observed output for this handoff.

Smallest credible repair proof: first run the focused prepared/assembly probes
above to confirm the branch sequence changed, then run
`ctest --test-dir build --output-on-failure -R '^(backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$'`
with output captured to `test_after.log`. Escalate to the supervisor-selected
backend subset after a code change because the likely owner is shared branch
emission rather than only this testcase.
