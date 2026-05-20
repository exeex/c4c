Status: Active
Source Idea Path: ideas/open/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Fixed-Offset Skip Boundary

# Current Packet

## Just Finished

Step 1 localized the fixed-offset skip boundary. Textual prepared BIR for the
Duff copy chain remains consecutive, e.g. `block_6` copies `addr %lv.a.0+0`
to `addr %lv.b.0+0`, `block_9` copies `addr %lv.a.0+2` to
`addr %lv.b.0+2`, and `block_10` copies `addr %lv.a.0+4` to
`addr %lv.b.0+4`. The first bad concrete owner is
`src/backend/prealloc/stack_layout/coordinator.cpp` in
`find_direct_frame_slot`, reached from `build_direct_frame_slot_access` while
building prepared addressing metadata. There, the parsed slot-slice suffix is
used as the comparison offset unit, so a BIR local address with
`base_name=%lv.a.0` and `byte_offset=2` resolves to prepared frame slot `#2`
instead of slot `#1`; likewise `+4` resolves to slot `#4` instead of `#2`.
The prepared addressing evidence is:
`block_6 inst_index=4 frame_slot=#0`, `block_9 inst_index=4 frame_slot=#2`,
`block_10 inst_index=4 frame_slot=#4`; destination stores similarly use
`#39`, `#41`, `#43`. Stack layout then maps those slots to generated offsets
`0`, `4`, `8` and `78`, `82`, `86`, so AArch64 selection and printing preserve
an already-skipped prepared-addressing fact.

## Suggested Next

Repair `find_direct_frame_slot`/direct frame-slot access construction so local
address byte offsets are resolved against actual prepared frame-slot byte
coverage for sliced local slots, not the numeric suffix alone.

## Watchouts

- Preserve idea 342's repaired latch behavior; do not reopen the duplicate
  decrement path unless fresh evidence shows it returned.
- Do not special-case `00143`, `.LBB` labels, block numbers, stack offsets,
  source lines, or emitted instruction spellings.
- Keep expectation, unsupported, runner, timeout, CTest registration, and
  proof-log policy unchanged.
- The AArch64 boundary was ruled out for this skip: `lower_memory_instruction`
  copies the prepared frame-slot id into `MemoryOperand`, applies stack layout,
  and `print_memory`/`memory_address` renders that byte offset unchanged. The
  generated evidence from the current probe is `.LBB1_27` copying `[sp]` to
  `[sp,#78]`, then `.LBB1_8` copying `[sp,#4]` to `[sp,#82]`, matching the
  already-skipped prepared addressing slots `#2` and `#41`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
was run with combined output written to `test_after.log`.

Result: build succeeded (`ninja: no work to do`), and the focused test
reproduced the existing residual:
`c_testsuite_aarch64_backend_src_00143_c` failed with `[RUNTIME_NONZERO]`
exit `1`. This is sufficient proof for Step 1 localization because the packet
only required identifying the first bad boundary, not repairing it.
