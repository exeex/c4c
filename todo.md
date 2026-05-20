Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 closed the reviewer coverage gap for the current dirty stdarg/byval
slice flagged in `review/326_stdarg_byval_route_review.md`. No new behavioral
scope was added in this packet.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` for the two
broad dispatch repairs that were previously covered only by generated-code
evidence:

- `predecessor_add_publication_preserves_rhs_register_before_target_clobber`
  exercises predecessor-edge Add publication where the RHS already lives in
  the destination GPR and the LHS must be loaded into that same destination.
  The fixture requires the RHS to be copied to scratch before the LHS load,
  then consumed by the final Add.
- `wide_local_load_publication_uses_latest_narrow_byte_store` exercises a wide
  local-load store whose latest same-lane source is a prior narrow byte copy
  from a prepared global symbol. The fixture requires the store publication to
  rematerialize the byte source instead of treating the later prepared wide
  frame-slot load as authoritative.

The existing dirty implementation repairs remain the same: Add/Sub publication
ordering preserves register-home source authority before target clobber, and
widened local-load publication can source the latest same-lane byte store. The
new focused tests are neutral dispatch fixtures and avoid `00204.c`-specific
names.

## Suggested Next

Continue Step 2 only if the supervisor keeps this inside the variadic/stdarg
owner family; otherwise split the remaining variadic cursor/format residual.
The new first bad fact has moved past the first `%9s` high-lane source. With
unbuffered runtime output, the program prints `stdarg:` and the first byval
payload bytes now begin as `ABCDEFGHI`, but the separator bytes between the
first `%9s` values are corrupted (`0xd0`, `0xd4`, `0xd8`, ...) and execution
segfaults before completing the first stdarg line. The next residual is no
longer the byval payload high lane; it is in the variadic/stdarg cursor or
format traversal after the repaired aggregate bytes are consumed.

## Watchouts

Do not revert the `x8` sret transport or the large-byval indirect pointer
classification to chase the HFA return failure. Those changes are what restore
the `cdefghijklmnopqrs` large-aggregate output and the aggregate string return
block.

Do not broaden this repair into an unconditional frame-slot retarget. The
current fix is intentionally scoped to frame-slot load results whose prepared
value feeds a same-block before-return FPR ABI register move. GPR scalar
returns must continue to use the return ABI register selected by return
lowering.

Do not undo the before-return frame-slot source support to chase the remaining
`00204.c` failure. The float/double HFA return-value lines now pass through
`fr_hfa14` and `fr_hfa24`; the remaining first bad fact is after the
return-value block, not the four-lane float/double ABI register lane
publication.

Do not undo the F128 pointer-value memory transport repair to chase the
remaining stdarg segfault. Long-double HFA returns now copy through `x8`, and
the next failure is after the return-value block has completed.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

Files touched by this packet: `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
and `todo.md`. The implementation files remain dirty from the prior
stdarg/byval repair slice.

## Proof

`git diff --check` passed.

Ran the delegated proof command:
`git diff --check`, then
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`, and
`backend_aarch64_return_lowering` passed.
`c_testsuite_aarch64_backend_src_00204_c` still failed with `RUNTIME_NONZERO`
/ segmentation fault. `test_after.log` is the fresh proof log for the delegated
build and CTest command.

The new first bad fact remains unchanged from the prior dirty stdarg/byval
slice: the failure is past the entry crash, the missing `x2` publication, and
the corrupt first `%9s` high-lane source. With unbuffered runtime output, the
program prints `stdarg:` and the first byval payload bytes begin as
`ABCDEFGHI`, but the separator bytes between the first `%9s` values are
corrupted (`0xd0`, `0xd4`, `0xd8`, ...) and execution segfaults before
completing the first stdarg line.
