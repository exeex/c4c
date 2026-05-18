# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Locate Scalar Authority Breaks

# Current Packet

## Just Finished

Step 1 reproduced the focused scalar representative subset and located the
first semantic repair target: AArch64 scalar move/materialization authority at
the prepared move-boundary plus scalar ALU fallback boundary. The next repair
should start in `src/backend/mir/aarch64/codegen/alu.cpp` and
`src/backend/mir/aarch64/codegen/calls.cpp`, where prepared scalar facts exist
but lowering either falls back to stale physical registers or rejects
non-register scalar sources instead of materializing the authoritative value.

Generated-code evidence:

- `src/00012.c`: BIR/prepared BIR folds `(2 + 2) * 2 - 8` to `%t2` with
  `storage %t2 immediate imm_i32=0` and a `before_return` move to `x0`, but
  generated assembly is `sub w0, w19, #8`. The result is scalar immediate
  return publication, not pointer/aggregate or printer admission.
- `src/00009.c`: BIR contains scalar `%t1 = mul`, `%t3 = sdiv`, `%t5 = srem`,
  and `%t7 = sub`; prepared storage assigns these scalar values, but generated
  assembly repeatedly stores stale `w19` to the local slot and only emits the
  final `sub w0, w13, #2`. This ties the failure to scalar result/local
  publication.
- `src/00056.c`: prepared callsite `printf("%d, %d\n", c, d)` has `arg2
  bank=gpr from=frame_slot:stack+24 to=x2` and a `call_arg_stack_to_register`
  move, but generated assembly sets only `x0` and `x1` before `bl printf`.
  The missing `x2` materialization explains `12, 0` and is scalar call-argument
  authority, while the format pointer is already valid.
- `src/00156.c` and `src/00161.c`: generated assembly reloads scalar locals but
  branch/control updates consume stale or unchanged values, matching the same
  scalar value publication family.
- `src/00211.c`: generated assembly calls `printf` with the valid format
  pointer in `x0` but never materializes scalar `n + 1` into an argument
  register, matching scalar call-argument authority.

## Suggested Next

Implement Step 2 for the smallest shared primitive: materialize authoritative
prepared scalar values for AArch64 move-boundary consumers before falling back
to physical-register guesses. Start with rematerializable-immediate and
stack-slot/register scalar sources feeding return ABI and call-argument ABI
destinations, then re-run the six-representative subset and inspect `00012.c.s`
and `00056.c.s` for `x0`/`x2` materialization.

## Watchouts

- Do not solve this by filename checks, expectation changes, allowlist changes,
  unsupported downgrades, or CTest edits.
- Keep pointer/aggregate address failures, timeout/hang cases, and
  compile-stage printer gaps out of this owner. This Step 1 sample did not
  require those buckets.
- `src/00056.c` confirms the third `printf` format pointer is correct; only the
  scalar second variadic value is missing. Treat it as scalar ABI argument
  materialization, not the closed string/global address owner.
- `src/00012.c` is the cleanest first proof for immediate return publication;
  `src/00056.c` is the cleanest first proof for stack-slot scalar call
  argument publication.

## Proof

Ran exactly:

```sh
ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00009|00012|00056|00156|00161|00211)_c$' -j 4 --timeout 5 --output-on-failure > /tmp/c4c_aarch64_scalar_step1_subset.ctest.log 2>&1
```

Result: failed as expected for the locating packet, with all six
representatives still failing. Log:
`/tmp/c4c_aarch64_scalar_step1_subset.ctest.log`.

Supporting transient dumps:

- `/tmp/c4c_aarch64_scalar_step1_00009.bir.txt`
- `/tmp/c4c_aarch64_scalar_step1_00009.prepared.txt`
- `/tmp/c4c_aarch64_scalar_step1_00012.bir.txt`
- `/tmp/c4c_aarch64_scalar_step1_00012.prepared.txt`
- `/tmp/c4c_aarch64_scalar_step1_00056.prepared.txt`
