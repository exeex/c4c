Status: Active
Source Idea Path: ideas/open/374_aarch64_local_aggregate_address_call_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative And Classify Guard

# Current Packet

## Just Finished

Executed idea 374 Steps 2 and 3 implementation packet. Added focused AArch64
dispatch coverage for direct local aggregate address call operands and
zero-offset computed local aggregate address call operands. Repaired scalar
pointer call-argument publication so register-homed local aggregate address
values materialize from the matching `%name.0` frame slot before the ABI move,
and extended frame-slot call-argument address publication so stack-homed local
aggregate pointer values also publish the local frame address instead of
reloading an uninitialized pointer home.

Generated publication facts after repair:

- `00218` `main`: the callsite now emits `add x21, sp, #0; mov x0, x21; bl
  convert_like_real`, so the stale `x21` pointer publication first bad fact is
  removed.
- `00216` `foo`: the first local aggregate print now emits `add x13, sp, #0`
  before `mov x1, x13; bl print_`, and following stack-homed local aggregate
  print operands now publish frame addresses such as `add x1, sp, #4` and
  `add x1, sp, #8` instead of `ldr x1, [sp,#...]`.

Proof did not close both representatives. `00218` still prints
`unsigned enum bit-fields broken`, but the first bad fact has moved past the
call-argument address: generated `main` stores `AMBIG_CONV` at `sp+2`, while
`convert_like_real` correctly reads/masks `[x0,#16]`. That residual is a
separate local aggregate/bit-field layout store boundary, not the local-address
call publication owner. `00216` still segfaults after the local aggregate
`print(...)` calls are address-published; the next visible suspicious region is
later compound-relocation/function-pointer lowering (`test_multi_relocs` still
selects/calls the table entry through `x21` regardless of the loop index).
Treat that as an advanced residual, not part of this local-address call packet.

## Refinement

Refined the uncommitted repair after the supervisor full-suite candidate found
that `00204` regressed from passing to a `myprintf` segfault. The first bad fact
was not the ordinary `printf("%.Ns", a.x)` by-value aggregate display path. It
was `myprintf`'s `llvm.va_start.p0` intrinsic call taking the new local
aggregate address publication shortcut: the fallback materialized the va_list
operand through the call-argument path and corrupted va_list setup, leaving the
overflow cursor null when later HFA `va_arg` logic loaded from it.

The repair now keeps local aggregate address publication scoped to non-intrinsic
pointer call operands and excludes ABI `byval_copy` operands. Focused dispatch
coverage was added for both guards: a byval aggregate pointer argument must not
take the local address fallback, and an `llvm.va_start.p0` operand must not use
the call-argument local address fallback. The intended idea 374 shapes remain
covered by the direct local aggregate address and zero-offset computed frame
address call-argument tests.

Generated `00204` now passes again. In `myprintf`, va_list initialization writes
the overflow, gp, and fp cursors into the va_list storage and no longer leaves
the overflow cursor null at the later HFA `va_arg` path. Protected
representatives `00182`, `00163`, `00151`, and `00214` remain green.

## Suggested Next

Stop this packet at the current proof result and hand lifecycle choice back to
the supervisor. The implemented local-address call publication repair is
covered and protected representatives remain green, but `00218` needs a
separate bit-field/local aggregate layout-store owner before this focused idea
can be closed. `00216` should remain a crash guard for the next aggregate
initializer/function-pointer residual if the supervisor chooses to continue
after the `00218` handoff.

## Watchouts

Do not broaden this repair into `00218` bit-field/local aggregate byte layout
or `00216` compound-relocation/function-pointer selected-table lowering inside
idea 374 without lifecycle approval. The local call-argument address publication
shape is now repaired for register homes and stack homes; further work should
start from the new first bad facts.

## Proof

Ran exact delegated refinement proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00218_c|c_testsuite_aarch64_backend_src_00216_c|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00163_c|c_testsuite_aarch64_backend_src_00151_c|c_testsuite_aarch64_backend_src_00214_c)$' > test_after.log 2>&1
```

Result: build completed; backend tests and protected representatives `00182`,
`00163`, `00151`, `00214`, and `00204` passed. `test_after.log` remains nonzero
because `00218` still has `RUNTIME_MISMATCH` and `00216` still has
`RUNTIME_NONZERO` segmentation fault after the localized local-address
publication gap was removed. Focused pre-proof check:
`backend_aarch64_instruction_dispatch` passed.

Supervisor full-suite candidate matched the accepted baseline after the
refinement: `3352` passed / `23` failed before and after, with no new failing
tests. A stale stopped `00204.c.bin` process from the run was killed before
continuing.
