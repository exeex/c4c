Status: Active
Source Idea Path: ideas/open/372_aarch64_pointer_valued_subobject_address_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Pointer-Local Address Publication

# Current Packet

## Just Finished

Refined idea 372 Step 3 again and refreshed Step 4 representative proof notes
for the second full-suite regression found after the `00151` refinement.

Coverage added:

- `backend_aarch64_instruction_dispatch` now also covers a stack-homed scalar
  compare result feeding a logical-not-style XOR consumer. The test requires
  the compare result to be published before any logical consumer can read the
  prepared stack home or reuse the live compare register.
- `backend_aarch64_instruction_dispatch` now covers a same-width `zext` stack
  publication for a logical-not value feeding a boolean branch path. The test
  requires the already-emitted logical value to be stored into the prepared
  stack home before any later branch materialization can observe or rederive
  the value.

Repair:

- Regression cause: the `00151` compare result `%t14` is intentionally
  stack-homed after the idea 372 pointer-address publication changes increased
  pressure. AArch64 compare publication emitted `cmp`/`cset` but discarded the
  prepared `result_stack_offset_bytes`, so the following logical-not path
  reloaded stale stack bytes from `[sp]`.
- Refinement: scalar compare publication now uses the existing scalar ALU stack
  publication helper to store stack-homed compare results immediately after
  `cset`.
- Second regression cause: `00214` lowered `!r` through a compare, XOR, and a
  prepared same-width `zext` stack home before branching. The stack publication
  for the no-op `zext i32 -> i32` re-materialized the logical-not expression
  instead of storing the already-emitted XOR value, which added a second
  boolean inversion and sent the non-null `r` path to `wrong4`.
- Refinement: same-width zero-extension stack publication now publishes the
  emitted scalar source register directly to the prepared stack home when that
  source is already available, avoiding duplicate logical-not materialization.
  The control cast helper also accepts same-width zero-extension as a valid
  no-op publication.

Representative results: `00151`, `00163`, and `00214` pass. Generated
`00151.c.s` still loads `arr+36` and `arr+96`, then emits `cmp w20, w21`,
`cset w9, eq`, `str w9, [sp]` before the logical-not path. Generated
`00163.c.s` still materializes `bolshevic + 4`, stores that computed pointer
into `[sp, #8]`, and reloads it before the final dereference. Generated
`00214.c.s` now computes `r != 0`, XORs once for `!r`, stores that value with
`str w13, [sp, #12]`, then reloads and branches; for non-null `r`, the branch
falls through to the `okay` arm.

## Suggested Next

Supervisor should run the broader/full regression guard before accepting idea
372 closure, since this packet repaired two full-suite regressions exposed by
previous narrow proofs.

## Watchouts

This refinement is not `00151`- or `00214`-shaped: it repairs the general
stack-homed scalar compare publication path and the same-width zero-extension
stack publication path. The pointer-local address publication behavior from
idea 372 remains intact for `00163`; closed ideas 294/355 and unrelated
external-call return, static selected arrays, aggregate initializer, FP,
bit-field, timeout, and idea 371 selected-aggregate buckets remain out of
scope.

## Proof

Delegated refinement command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00163_c|c_testsuite_aarch64_backend_src_00151_c|c_testsuite_aarch64_backend_src_00214_c)$' > test_after.log 2>&1
```

Result: build completed; CTest selected 146 tests and all 146 passed,
including local `backend_.*` coverage,
`c_testsuite_aarch64_backend_src_00163_c`,
`c_testsuite_aarch64_backend_src_00151_c`, and
`c_testsuite_aarch64_backend_src_00214_c`. Proof log is `test_after.log`.
