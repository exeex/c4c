Status: Active
Source Idea Path: ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Scalar Formal Boundary

# Current Packet

## Just Finished

Step 1 localized the scalar formal preserved-home/post-call reuse boundary for
`00181` after idea 357's pointer-formal repair.

The scalar formal is `Hanoi`'s `%p.n` (`int n`, value id 101). Prepared BIR
keeps `%p.n` as the source for both recursive decrements:
`%t4 = bir.sub i32 %p.n, 1` before the first recursive call and
`%t6 = bir.sub i32 %p.n, 1` after the intervening recursive `Hanoi` call and
`Move` call. The prepared call plans preserve `%p.n` through stack slot
`slot#72` at `stack+0` (`size=4 align=4`), and the prepared move bundle before
`block_index=2 instruction_index=3` records a `consumer_stack_to_register`
move from `%p.n` to `%t6`.

Generated AArch64 for `Hanoi` still lowers that post-call consumer as
`sub w13, w0, #1` immediately after `bl Move` in `.LBB194_29`. At that point
`w0` is the `Move` return/call-result register, not the incoming scalar formal.
The consuming lowered instruction path is therefore the second recursive
call's `n - 1` argument materialization: `sub w13, w0, #1`; `mov w0, w13`;
`bl Hanoi`. It should reload `%p.n` from the preserved stack home at `stack+0`
or otherwise publish that preserved home before this post-call scalar use.

Pointer-formal publication remains repaired and out of scope: generated code
publishes `%p.source`/`%p.dest` into `x20`/`x21` before the recursive and
`Move` call-argument consumers (`mov x20, x1`, `mov x21, x2`, then
`mov x0, x20`/`mov x1, x21` as needed).

## Suggested Next

Execute Step 2: repair post-call scalar formal consumers so `%p.n`-like
prepared preserved homes are reloaded or published before consumers that run
after clobbering calls.

## Watchouts

- Do not reopen idea 357 pointer-formal callee-saved home publication unless
  fresh evidence shows the split was wrong.
- Reject named-case, register-only, stack-offset-only, or `Hanoi`-specific
  fixes.
- Keep semantic-BIR pointer-derived string loads for `00173` out of scope.
- The first bad fact is not the semantic BIR expression: prepared BIR still
  correctly models `%t6` as `%p.n - 1`. The stale value is introduced when
  lowering the prepared `consumer_stack_to_register` obligation for the
  post-call `%t6` consumer.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: 6/7 passed. Backend contracts passed, `00170` and `00189` passed, and
`c_testsuite_aarch64_backend_src_00181_c` remains the single expected
`RUNTIME_NONZERO` segmentation-fault residual for Step 2.

Proof log: `test_after.log`.
