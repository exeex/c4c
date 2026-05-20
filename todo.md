Status: Active
Source Idea Path: ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Guard And Closure Decision

# Current Packet

## Just Finished

Step 4 reclassified the residual `00182` failure after the unsigned div/rem
repair.

The unsigned div/rem producer publication fix remains in place and the focused
backend coverage stays green. `00182` advanced past the original boundary:
generated assembly now contains `udiv`/`msub` for `x % 10L` around lines
553-557 and `udiv` plus `str x20, [sp]` for `x / 10L` around lines 971-974 in
`build/c_testsuite_aarch64_backend/src/00182.c.s`.

Precise new first bad fact: the residual segfault is caused by frame-size/local
array storage, not unsigned div/rem publication. In
`tests/c/external/c-testsuite/src/00182.c`, `main` declares
`char buf[5*MAX_DIGITS]` with `MAX_DIGITS == 32`, so the buffer needs 160
bytes. Generated `main` allocates only 48 bytes (`sub sp, sp, #48`), passes
`sp` as the buffer argument to `print_led`, and saves `x30` at `[sp, #24]`.
Once digit extraction now reaches the printing loops, `print_led` writes the
LED text into that undersized caller frame and overwrites main's saved return
address with output bytes. GDB shows `pc`/`x30` corrupted with ASCII output
data (`0x20200a20205f20`, `0x7c20200a20205f20`).

Blocker: making `00182` pass now requires repairing local aggregate/array frame
layout or stack allocation sizing for caller local buffers. That is outside
idea 350's unsigned div/rem producer-publication owner and should be split or
reviewed before implementation continues.

## Suggested Next

Lifecycle decision: accept the uncommitted unsigned div/rem implementation and
focused tests as scoped progress for idea 350 despite the strict no-pass-count
guard failure, provided supervisor review confirms the diff is not
testcase-overfit and the recorded focused backend proof remains fresh. The
source idea's acceptance criteria require `00182` to advance past stale
unsigned div/rem producer publication or be reclassified by a new first bad
fact; this packet did that. Do not rework idea 350 solely to force a CTest pass
count increase through the newly exposed local-array frame-size failure.

The residual `00182` segfault should be owned outside idea 350. Use existing
open idea `ideas/open/316_aarch64_frame_slot_layout_consistency.md` as the
focused frame-size/frame-slot owner; it has been reactivated with the fresh
`00182` local array evidence instead of creating a duplicate new idea.

Supervisor next action for idea 350: run the selected broader guard/reviewer
check for overfit and adjacent scalar publication stability. If accepted,
commit the implementation/test changes with this `todo.md` progress. If that
review finds testcase-shaped matching, expectation weakening, or stale
unsigned div/rem consumers still present, reject the slice and rework within
idea 350.

Supervisor acceptance update: lifecycle accepted this as scoped progress for
idea 350 because `00182` advanced to a new frame-layout first bad fact after
the unsigned div/rem repair. The strict subset regression guard remained 3/4
and failed to produce a pass-count increase, so acceptance is based on the
recorded first-bad-fact advance rather than strict guard improvement.

## Watchouts

- Do not special-case `00182`, the LED digit array, a temporary name, one
  register, or one emitted instruction sequence.
- Do not widen into recursive call argument preservation for `00176`/`00181`
  or indexed aggregate selected-address/writeback from idea 348.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- Do not try to make `00182` pass inside idea 350 by special-casing the LED
  buffer or by weakening expectations. The remaining failure is caller frame
  allocation for local arrays.

## Proof

Ran the delegated proof command exactly:

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_scalar_alu_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00182_c)$' | tee test_after.log`

Result: build passed; focused backend tests
`backend_aarch64_prepared_scalar_alu_records`,
`backend_aarch64_machine_printer`, and
`backend_aarch64_instruction_dispatch` passed. `00182` still failed, now as
`RUNTIME_NONZERO` with `Segmentation fault`; baseline `test_before.log` had
the same test red as `RUNTIME_MISMATCH`. This is a strict first-bad-fact
advance, not a pass-count improvement because the subset remains 3/4. Lifecycle
acceptance for idea 350 therefore depends on scoped-progress review rather than
the strict regression-count guard. Proof log path: `test_after.log`.

Supervisor acceptance validation: strict subset regression guard remained 3/4
and failed the pass-count-increase criterion, but lifecycle accepted the scoped
progress because `00182` advanced to the frame-layout/local-array first bad
fact described above. Broader backend validation PASS:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 141/141.
