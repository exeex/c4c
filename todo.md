Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select The Next Focused Owner

# Current Packet

## Just Finished

Step 3: Select The Next Focused Owner gathered `00143` evidence without
editing implementation, tests, plans, ideas, expectations, runner scripts, or
canonical proof logs.

Conclusion: `00143` is genuinely the same first bad fact as closed idea 338's
scalar-cast register-source operand owner, not a narrower idea-339 local
writeback/sizing residual. The next focused owner should reopen or split a
scalar-cast register-source publication owner, scoped tightly around selected
scalar cast nodes whose source value is stack-homed and has a prepared
stack-to-register consumer move that the machine printer still does not see as
a structured register source.

Evidence:

- Existing `test_after.log` and a fresh narrow CTest probe both fail
  `c_testsuite_aarch64_backend_src_00143_c` before assembly is emitted:
  `[FRONTEND_FAIL]`, machine-node printer reached, function 0 block 28
  instruction 158, `family=scalar opcode=sign_extend`, with
  `scalar cast node requires a structured register source operand`.
- Fresh assembly probe
  `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143.s`
  exits 1 with the same diagnostic and produces no assembly file. This keeps
  the first bad fact at selected-machine-node printing, not assembler/linker or
  runtime behavior.
- Fresh prepared dump
  `/tmp/c4c_00143.prep_block16.txt` shows the failing compare tail in
  `block_16`: `%t76 = bir.select ... i16`, then
  `%t81 = bir.sext i16 %t76 to i32`; `%t81` is the scalar cast corresponding
  to the printer's `sign_extend`.
- The same prepared metadata records `%t76 value_id=308` as `kind=stack_slot`
  and `%t81 value_id=388` as `kind=register reg=x13`. The move bundle before
  block index 28 instruction 158 contains
  `move from_value_id=308 to_value_id=388 destination_storage=register
  reason=consumer_stack_to_register`.
- Therefore the prepared route has evidence of the needed register materialize
  move, but the selected AArch64 scalar cast machine node still reaches the
  printer without a structured register source operand. That is the direct
  register-source operand-fact failure described by idea 338, narrowed to the
  post-regalloc selected-source publication path for a stack-homed `i16`
  select feeding `sext`.

## Suggested Next

Ask the plan owner to split a focused scalar-cast register-source owner for
`00143`, with representative proof starting at the same narrow CTest and
prepared/asm probes. Scope it to publishing structured register sources for
selected scalar casts after consumer stack-to-register moves; do not fold in
the idea-339 local storage/writeback sizing owner or the runtime/timeout
residual buckets.

## Watchouts

- Do not implement under the umbrella inventory plan.
- Do not frame `00143` as an idea-339 downstream local writeback residual; the
  first bad fact is still the scalar cast register-source printer contract.
- Keep the owner semantic and operand-fact based. Avoid `00143`-specific
  matching, diagnostic-string matching, or instruction-number matching.
- The prepared stack-to-register move exists, so the likely boundary is
  publication/selection of the cast source operand, not absence of regalloc
  movement.
- Treat timeout and output-storm cases as unsafe for broad reruns unless a
  later delegated command includes timeout and stale-process cleanup.
- Do not change expectations, runner behavior, unsupported classifications, or
  CTest registration to improve backend-regex counts.
- The remaining runtime mismatch/nonzero and timeout buckets remain parked
  until generated-code, prepared-state, or timeout-safe probes justify their
  own owners.

## Proof

Used existing canonical proof log plus focused non-mutating probes. Did not
rewrite `test_after.log`.

Existing canonical proof log: `test_after.log`.

```sh
ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c' > /tmp/c4c_00143.ctest.txt 2>&1 || true
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c > /tmp/c4c_00143.prepared.txt 2> /tmp/c4c_00143.prepared.err
./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main --mir-focus-block block_16 tests/c/external/c-testsuite/src/00143.c > /tmp/c4c_00143.prep_block16.txt 2> /tmp/c4c_00143.prep_block16.err
./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143.s > /tmp/c4c_00143.asm.stdout 2> /tmp/c4c_00143.asm.stderr
```

Focused probe artifacts:

- `/tmp/c4c_00143.ctest.txt`: narrow CTest reproduces the same scalar
  `sign_extend` structured register source diagnostic.
- `/tmp/c4c_00143.prepared.txt`: full prepared dump succeeds.
- `/tmp/c4c_00143.prep_block16.txt`: focused prepared block evidence for
  `%t76`, `%t81`, value ids 308/388, and the consumer stack-to-register move.
- `/tmp/c4c_00143.asm.stderr`: asm probe exits 1 with the same printer
  diagnostic; `/tmp/c4c_00143.s` is not produced.
