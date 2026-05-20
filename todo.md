Status: Active
Source Idea Path: ideas/open/336_aarch64_return_result_publication_epilogue_clobber.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Return Result Clobber Sites

# Current Packet

## Just Finished

Step 1 localization completed with generated assembly plus prepared-BIR
evidence. No implementation or test files were changed.

Representative first bad facts:

- No-call scalar `00011`: source returns local `x == 0`. Prepared BIR has
  `bir.ret i32 %t0`, stores `%t0` in home `x13`, and publishes it through a
  `before_return` move to `x0`. Generated assembly instead loads the result
  directly into `w0` (`ldr w0, [sp]`) and immediately overwrites it with the
  prepared home copy (`mov x0, x13`). There is no saved-register restore in
  this function, so this is not epilogue/restore ordering.
- Pointer/local `00004`: prepared BIR returns `%t2`, whose home is callee-saved
  `x20`, with a `before_return` move to `x0`. Generated assembly loads the
  correct local value into `w0`, then emits `mov x0, x20` before restoring
  `x20`. The first bad fact is again producer/result-publication authority
  diverging from the prepared return home; `x20` never receives the loaded
  `%t2` value before the return move consumes it.
- Call-result `00087`: prepared BIR preserves the indirect call result from
  `x0` to `%t3` home `x21`, then has a `before_return` move from `%t3` to
  `x0`. Generated assembly performs `blr x13`, `mov x21, x0`, and `mov x0,
  x21`, but then restores `x21` and emits a second terminal return move
  `mov w0, w21`. The call-result preservation itself is present; the bad fact
  is duplicate terminal return publication after epilogue insertion from a
  callee-saved home that has already been restored.
- Additional call/ALU shape `00168`: recursive `factorial` preserves the
  recursive call result from `x0` to `x21`, computes the intended product with
  `mul w0, w19, w21`, then overwrites it with `mov x0, x13`. Prepared BIR says
  the product `%t5` home is `x13` and the `before_return` move publishes
  `%t5` to `x0`; generated scalar-ALU lowering instead wrote the product
  directly to the return ABI register without making that publication
  authoritative.

Owner decision: the narrow owner is AArch64 return-result publication
coordination, not return home selection, not call-result preservation, and not
raw epilogue/restore ordering alone. Prepared homes and `before_return`
bundles are internally coherent, and call-result moves to callee-saved homes
are present. The broken boundary is that return-feeding producers can publish
directly to `w0`/`x0` while the prepared home-to-return move remains live, and
the terminal `ReturnInstructionRecord` can emit another home-to-return move
after frame teardown.

Narrow repair target: make AArch64 return lowering use a single authoritative
return-result publication path for `FunctionReturnAbi`. Either keep producer
results in their prepared homes and let the `before_return` move publish them,
or make direct producer-to-ABI publication authoritative and suppress the later
prepared/terminal duplicate moves. In either route, terminal return printing
must not emit a post-epilogue register move from a callee-saved home when the
`before_return` bundle already published the result.

## Suggested Next

Execute Step 2 from `plan.md`: repair the AArch64 return publication
coordination path so load/scalar-ALU return producers and call-result returns
do not leave a stale prepared-home move or a post-epilogue terminal return move
that overwrites `w0`/`x0`. Start around the AArch64 codegen helpers that
retarget return-feeding producers to the return ABI and the return terminator /
before-return move emission path.

## Watchouts

Do not fix this by deleting all `before_return` moves or by special-casing the
c-testsuite filenames. The no-call scalar and pointer/local cases prove the
bug is not only epilogue ordering; the `00087` call-result case proves the
terminal return record can still emit after frame teardown even when
`before_return` publication was already correct. Keep closed ideas 333 and 335
out of scope unless fresh generated-code evidence ties the repair to their
exact old owner.

## Proof

`git diff --check`. Formatting-only proof for the `todo.md` evidence update;
no code/test validation was run in this localization packet.
