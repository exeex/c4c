Status: Active
Source Idea Path: ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Stack-Preserved Pointer Formal Boundary

# Current Packet

## Just Finished

Plan Step 1 localized the stack-preserved pointer-formal residual without
implementation edits. In `Hanoi`, pointer formal `%p.spare` is value_id `104`;
prepared BIR preserves it through stack slot `#73` / object `#74`, stack offset
`+8`, size 8. The post-call overwrite boundary is the non-base path after the
first recursive `Hanoi(n-1, source, spare, dest)` and the following
`Move(source, dest)`: emitted AArch64 stores stale caller-clobbered `x3` back
to `[sp, #8]` immediately before the final recursive call setup, corrupting the
prepared home that should still hold `%p.spare`.

## Suggested Next

Repair AArch64 call-boundary lowering so stack-preserved pointer formals are
not republished from caller-clobbered ABI argument registers after intervening
calls; the first focused guard should cover a stack-preserved pointer formal
consumed by a later recursive/helper call after another call clobbers its
original ABI register.

## Watchouts

- Prepared BIR evidence from
  `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function Hanoi tests/c/external/c-testsuite/src/00181.c`:
  `%p.spare` starts in `x3`, is assigned preserved stack slot `#73` at
  `stack+8`, and is consumed as a stack-to-register call argument at block 2
  inst 1 arg2 and block 2 inst 4 arg1. The adjacent same-shape initial spill
  before the first recursive call is correct; the bad same-slot store happens
  only after intervening calls.
- Generated AArch64 evidence from `/tmp/00181_aarch64.s`: line 442 spills
  incoming `x3` to `[sp, #8]`; line 449 calls `Hanoi`; lines 450-452 set up and
  call `Move`; line 456 overwrites `[sp, #8]` from stale `x3`; line 458 reloads
  `[sp, #8]` into `x1` for the final lowered `Hanoi(n-1, spare, dest, source)`
  path at line 461.
- Scalar `%p.n` remains a separate stack slot `#72` / `stack+0` and is reloaded
  at the later decrement, so this packet did not find fresh evidence to reopen
  idea 358. `%p.source` and `%p.dest` remain callee-saved in `x20`/`x21`, so
  this packet did not find fresh evidence to reopen idea 357.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: build was up to date; 6/7 selected tests passed. The only failure was
`c_testsuite_aarch64_backend_src_00181_c`, still `RUNTIME_NONZERO` with
`exit=Segmentation fault`. Proof log: `test_after.log`.
