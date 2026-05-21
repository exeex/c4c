Status: Active
Source Idea Path: ideas/open/357_aarch64_recursive_pointer_formal_home_publication.md
Source Plan Path: plan.md
Current Step ID: 2-3
Current Step Title: Add Focused Recursive Pointer-Formal Coverage + Repair Pointer-Formal Home Publication

# Current Packet

## Just Finished

Implemented `plan.md` Steps 2 and 3 for pointer-formal callee-saved home
publication.

Added focused backend coverage for a sibling-control-flow shape where a prior
call in one branch preserves `%arg` in `x20`, while the consumer call in the
other branch must not assume that prior call populated `x20`. The test now
requires the consumer block to emit `x1 -> x20` before the `x20 -> x0`
call-argument move.

Repaired the preserved-home population decision so prior callee-saved
preservation suppresses a fresh population only when the prior call is in the
same block before the current call or its block dominates the current block.
The before-call move ordering also keeps
`callee_saved_preservation_home_population` before consumers of that home.

Generated `00181` AArch64 advanced past the pointer-formal first bad fact: the
recursive block now emits `mov x20, x1` and `mov x21, x2` before the first
recursive/helper-call consumers use `x20`/`x21`.

## Suggested Next

Classify and repair the remaining `00181` residual if the supervisor keeps it
inside this owner: after the first recursive/helper calls, generated AArch64
still computes the later `n - 1` from clobbered `w0` instead of the preserved
`%p.n` stack home.

## Watchouts

- The remaining `00181` failure is no longer the original `x20`/`x21`
  pointer-formal home issue. Current generated code stores `%p.spare` at
  `stack+8`, publishes `%p.source`/`%p.dest` to `x20`/`x21`, and still later
  emits `sub w13, w0, #1` after calls that clobber `w0`.
- Do not special-case `00181`, `Hanoi`, `Move`, one register, one stack offset,
  or one emitted instruction neighborhood.
- Keep `00170` and `00189` in the regression subset for call-publication work.
- Keep `00173` out of this owner; it belongs to semantic-BIR pointer-derived
  string-load work.

## Proof

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: failed with forward progress, 6/7 passed. Backend contracts passed,
including the new focused dispatch coverage. Idea 355 representatives `00170`
and `00189` remained passing. The only failing test was
`c_testsuite_aarch64_backend_src_00181_c`, still `RUNTIME_NONZERO` with a
segmentation fault after the pointer-formal `x20`/`x21` publication boundary
was repaired.

Proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
