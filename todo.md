Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Select the Next Semantic Owner

# Current Packet

## Just Finished

Step 2: Select the Next Semantic Owner classified the remaining post-293
non-frontend failures from the reused inventory scan and selected the next
focused owner:

`AArch64 pointer-derived address/lvalue lowering authority`.

The selected owner should cover pointer-derived addresses used for loads,
stores, argument publication, and compound lvalue updates after array decay or
pointer arithmetic. Starter representatives:

- `src/00217.c`: global string data, pointer offset, cast to `unsigned *`, and
  `*(unsigned *)(data + r) += a - b`; generated AArch64 stores only `a` to
  `t+4`, ignores the original loaded value and `b`, then prints through an
  uninitialized pointer slot.
- `src/00032.c`: local `int arr[2]` plus post/pre-increment pointer
  dereferences; generated AArch64 reads pointer values from out-of-frame stack
  offsets such as `[sp, #64]` instead of deriving `&arr[0]` / `&arr[1]`.
- `src/00130.c`: two-dimensional local char array, pointer-to-array, scalar
  `char *`, and `*q`; generated AArch64 reads `arr[1][3]` from `[sp, #2]`,
  showing broken array-element address scaling/offset authority.
- `src/00180.c`: local char buffer passed to `strcpy` and then printed from
  `&a[1]`; generated AArch64 calls `strcpy` with uninitialized `x20` instead
  of the local buffer address.

This is a semantic family rather than a named c-testsuite repair because the
same ownership question appears across local arrays, pointer arithmetic,
array-to-pointer decay, pointer-to-array indexing, string-buffer external calls,
and global object subobject stores.

```sh
ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure > /tmp/c4c_aarch64_post293_inventory_scan.log 2>&1
```

Rejected/deferred buckets:

- `FRONTEND_FAIL` 52-case bucket: deferred as frontend/admission work, not a
  backend semantic owner for this focused split.
- `TIMEOUT src/00220.c`: deferred because it needs timeout-specific handling
  and stale-process discipline before treating it as ordinary runtime behavior.
- Floating/conversion cases `src/00174.c`, `src/00175.c`, and `src/00119.c`:
  deferred as a separate floating/scalar-conversion owner.
- String/library-only surface such as `src/00123.c`, `src/00186.c`, and
  `src/00189.c`: deferred until pointer-derived address publication is either
  fixed or ruled out, because current representatives include uninitialized or
  wrong buffer/function-pointer argument addresses.
- Aggregate/global initializer case `src/00050.c`: deferred as aggregate
  layout/initializer work unless later evidence connects it directly to the
  selected pointer-derived lvalue path.

Closed-owner overlap rationale:

- `src/00159.c`, `src/00168.c`, `src/00193.c`, and `src/00196.c` stay
  closed-owner overlap for call value, recursion/control value, switch/side
  effect publication, or logical-control behavior unless fresh generated-code
  evidence contradicts closed owners 285-293.
- The selected owner does not reopen the closed non-leaf call-frame, scalar
  call value, string/global external call, SP alignment, function-pointer call
  value, scalar parameter ALU, call-argument register, scalar expression
  control-value, or side-effect control-value publication owners. It is limited
  to deriving and using concrete memory addresses for pointer/array lvalues.

## Suggested Next

Have the plan owner split a focused idea for `AArch64 pointer-derived
address/lvalue lowering authority`, with `00217`, `00032`, `00130`, and `00180`
as starter representatives and nearby sampling from `00019`, `00137`, `00138`,
and pointer-heavy string/indirect-call cases after the first fix.

## Watchouts

- Keep the focused idea about semantic address derivation and lvalue use, not
  testcase-shaped rewrites for `00217`.
- The proof should include compile/assembly inspection for emitted addresses,
  not just runtime pass/fail, because several current failures publish
  uninitialized registers as pointers.
- Preserve the post-293 separation for closed-owner overlap cases; do not use
  this owner to reopen side-effect/control-value work without contradictory
  evidence.

## Proof

Inventory-analysis-only packet. Reused supervisor-run log
`/tmp/c4c_aarch64_post293_inventory_scan.log`; tests were not rerun and no new
`test_after.log` was produced. Representative source and generated AArch64
assembly were inspected under `tests/c/external/c-testsuite/src/` and
`build-aarch64-scan/c_testsuite_aarch64_backend/src/`.

Expected focused-proof shape: a narrow CTest subset for
`c_testsuite_aarch64_backend_src_00217_c`,
`c_testsuite_aarch64_backend_src_00032_c`,
`c_testsuite_aarch64_backend_src_00130_c`, and
`c_testsuite_aarch64_backend_src_00180_c`, plus before/after generated assembly
checks showing correct local/global object address derivation, pointer
increment/decrement scaling, subobject offsets, and external-call pointer
arguments.
