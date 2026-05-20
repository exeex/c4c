Status: Active
Source Idea Path: ideas/open/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Fallthrough Copy Fixed-Offset Emission

# Current Packet

## Just Finished

Step 2 repaired prepared direct frame-slot lookup for dense sliced local slot
families. `find_direct_frame_slot` now resolves local-address byte ranges
against each candidate slice's byte coverage, derived from consecutive slice
slot sizes when suffixes form a dense ordinal family, instead of treating the
numeric suffix itself as the byte offset. Focused `backend_prepare_stack_layout`
coverage now proves `%slice.0+2` and `%slice.0+4` resolve to the second and
third two-byte frame slots with zero intra-slot offset.

For `00143`, prepared addressing for the Duff fallthrough chain no longer skips
every other slot: `block_6` uses source/destination slots `#0/#39`, `block_9`
uses `#1/#40`, and `block_10` uses `#2/#41`. Generated AArch64 likewise emits
fallthrough short-copy offsets `[sp]`, `[sp,#2]`, `[sp,#4]`, ... to
`[sp,#78]`, `[sp,#80]`, `[sp,#82]`, ... .

## Suggested Next

Run Step 3 as a residual-localization packet: after the fixed-offset skip
repair, classify why the Duff loop still repeats the same base slice range on
later iterations instead of advancing through the loop-carried `from`/`to`
pointer state.

## Watchouts

- The fixed-offset skip boundary localized in Step 1 is repaired; do not reopen
  it unless fresh prepared-addressing evidence again shows `+2 -> #2` or
  `+4 -> #4`.
- `c_testsuite_aarch64_backend_src_00143_c` still fails `[RUNTIME_NONZERO]`
  exit `1`. The smallest remaining observed fact is outside the repaired
  prepared slot lookup: the generated Duff body repeats fixed base copies from
  `a[0..7]` to `b[0..7]` across later loop iterations rather than consuming the
  advanced `from`/`to` pointers for `a[8..38]` and `b[8..38]`.
- Preserve idea 342's repaired latch behavior; the latch still branches on the
  single post-decrement counter value.
- Do not special-case `00143`, labels, block numbers, stack offsets, source
  lines, emitted instruction strings, expectations, unsupported lists, runner,
  timeout, CTest, or proof-log policy.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^backend_|c_testsuite_aarch64_backend_src_00143_c)'`
was run with combined output written to `test_after.log`.

Result: build succeeded and backend coverage passed. The only failing test was
`c_testsuite_aarch64_backend_src_00143_c`, still `[RUNTIME_NONZERO]` exit `1`;
`test_after.log` is the canonical proof log for this packet.
