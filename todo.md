Status: Active
Source Idea Path: ideas/open/357_aarch64_recursive_pointer_formal_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Formal-Home Boundary

# Current Packet

## Just Finished

Localized `plan.md` Step 1 for `c_testsuite_aarch64_backend_src_00181_c`.
The smallest representative is `Hanoi(int n, int *source, int *dest,
int *spare)` in `tests/c/external/c-testsuite/src/00181.c`.

Prepared BIR assigns incoming formals to ABI registers:

- `%p.n` value_id 101 in `w0`
- `%p.source` value_id 102 in `x1`
- `%p.dest` value_id 103 in `x2`
- `%p.spare` value_id 104 in `x3`

The prepared callsite preservation plan for `Hanoi` says later same-module
calls preserve:

- `%p.source` through callee-saved home `x20`
- `%p.dest` through callee-saved home `x21`
- `%p.spare` through stack home `stack+8`
- `%p.n` through stack home `stack+0`

Generated AArch64 for the base-case block publishes `x1 -> x20` and `x2 ->
x21` immediately before `Move`, then consumes `x20`/`x21` as `Move` arguments.
The recursive block does not publish incoming `x1`/`x2` to `x20`/`x21` before
using those homes:

- first recursive `Hanoi(n - 1, source, spare, dest)` consumes `x20` as arg1
  and `x21` as arg3
- intervening `Move(source, dest)` consumes `x20`/`x21`
- second recursive `Hanoi(n - 1, spare, dest, source)` consumes `x21` as arg2
  and `x20` as arg3

First repair boundary: path-sensitive publication of incoming pointer formals
into prepared callee-saved homes before any block consumes those homes for a
same-module call or helper call. This is not call-argument selection: the
recursive call order matches semantic BIR, but selected arguments read stale
homes. It is not symbol materialization or idea 355 address-valued publication:
the bad values are incoming pointer formals, not global/string addresses or
pointer-valued loads. It is not semantic BIR: semantic/prepared BIR have the
correct `Hanoi` calls and a concrete preservation plan.

## Suggested Next

Add focused backend coverage for a same-module or recursive path where incoming
pointer formals assigned to `x20`/`x21` are published before any later
call-argument reload consumes those homes.

## Watchouts

- Do not edit implementation files, tests, expectations, runners, timeout
  policy, CTest registration, or proof-log behavior during localization.
- Do not special-case `00181`, `Hanoi`, `Move`, one callee-saved register, one
  stack offset, or one emitted instruction neighborhood.
- Keep `00170` and `00189` in the regression subset once code changes touch
  call publication.
- Keep `00173` out of this owner; it belongs to semantic-BIR pointer-derived
  string-load work.

## Proof

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: failed as expected for localization, 6/7 passed. Backend contracts and
idea 355 representatives `00170` and `00189` passed. The only failing test was
`c_testsuite_aarch64_backend_src_00181_c`, still `RUNTIME_NONZERO` with a
segmentation fault.

Proof log: `test_after.log`.
