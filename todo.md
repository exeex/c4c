Status: Active
Source Idea Path: ideas/open/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Representative And Reclassify Residuals

# Current Packet

## Just Finished

Step 3 ran the delegated representative proof and reclassified the residual
after the fixed-offset skip repair. The fixed-offset repair remains good:
prepared addressing still maps the Duff fallthrough copies consecutively
(`block_6` source/destination slots `#0/#39`, `block_9` `#1/#40`, `block_10`
`#2/#41`), and generated AArch64 emits consecutive short-copy offsets `[sp]`,
`[sp,#2]`, `[sp,#4]`, ... to `[sp,#78]`, `[sp,#80]`, `[sp,#82]`, ... .

The Duff latch remains the idea 342 form: semantic/prepared BIR uses one
post-decremented `%t34` for both `bir.store_local %lv.n, i32 %t34` and
`bir.sgt i32 %t34, 0`; generated AArch64 similarly computes one `w13 = n - 1`,
stores it to `[sp,#176]`, and branches on `cmp w13, #0`.

The smallest new first bad fact is already present in semantic BIR, before
prepare or AArch64 emission. The Duff copy blocks update `%lv.from` and
`%lv.to`, but the actual dereferences are frozen to direct array-base slots:
`block_6` loads `%lv.a.0` and stores `%lv.b.0`; `block_9` loads
`addr %lv.a.0+2` and stores `addr %lv.b.0+2`; the chain continues through
`block_15` at `+14`. On latch back to `block_6`, the stored loop-carried
pointer locals are not consumed, so later iterations repeat `a[0..7] ->
b[0..7]` instead of copying `a[8..38] -> b[8..38]`.

## Suggested Next

Treat the current runbook as exhausted for fixed-offset skip repair and route
the remaining representative failure as a separate semantic BIR
pointer-provenance/lower-memory packet. Start from `*from++`/`*to++` lowering
where a local pointer initialized from an array base is incremented across a
loopback; likely owner probes are `src/backend/bir/lir_to_bir/memory/addressing.cpp`
`lower_memory_gep_inst`, `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
`try_lower_local_slot_pointer_gep`, and `src/backend/bir/lir_to_bir/memory/provenance.cpp`
pointer-provenance load/store helpers.

## Watchouts

- Do not reopen fixed direct frame-slot lookup unless fresh evidence again
  shows prepared addressing skips the consecutive slot chain.
- The representative still fails `[RUNTIME_NONZERO]` exit `1`, but the new
  evidence is outside Duff fallthrough fixed-offset emission: semantic BIR
  direct-slot dereferences ignore loop-carried `%lv.from`/`%lv.to` state.
- Preserve idea 342's repaired latch behavior; this packet reconfirmed the
  latch branches on the single post-decrement counter value.
- Do not special-case `00143`, labels, block numbers, stack offsets, source
  lines, emitted instruction strings, expectations, unsupported lists, runner,
  timeout, CTest, or proof-log policy.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
was run with combined output written to `test_after.log`.

Result: build succeeded; `c_testsuite_aarch64_backend_src_00143_c` failed
`[RUNTIME_NONZERO]` exit `1`. `test_after.log` is the canonical proof log for
this packet. Additional local probes regenerated semantic BIR, prepared BIR,
and AArch64 assembly under `/tmp` for classification only.
