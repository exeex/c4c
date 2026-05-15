Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Intrinsic Authority And Fail-Closed Gaps

# Current Packet

## Just Finished

Step 1: `Inventory Intrinsic Authority And Fail-Closed Gaps`.

Inventory found that current intrinsic authority is call-shaped, not
intrinsic-record-shaped:

- BIR carries runtime/intrinsic placeholders as `bir::CallInst` with callee
  spelling, typed args/results, ABI info, side-effect metadata only for inline
  asm, and invalid link-name identity for synthesized runtime placeholders.
- Prepared state publishes ordinary `PreparedCallPlan` facts for these calls,
  plus separate variadic-entry helper facts and existing runtime-helper routes
  for i128/f128. It does not publish a structured AArch64 intrinsic carrier.
- AArch64 selection has structured records for ordinary calls, variadic helpers,
  f128/i128 helpers, atomics, memory, casts, and scalar ALU. It has no selected
  intrinsic record family yet.
- Memory intrinsics (`memcpy`/`memset`) already lower as ordinary memory
  operations before this route; they are not the first intrinsic carrier
  boundary.
- Scalar `llvm.fabs.{float,double,f128,x86_fp80}` has the strongest current
  semantic shape because BIR lowering preserves typed operand/result facts as a
  call placeholder. F128 remains delegated to the closed binary128 route.
- Barrier/cache/pause/hint, CRC, builtin-address, vector-memory, and vector-op
  families lack enough upstream semantic carrier facts for responsible AArch64
  selected-node support in this packet. They should either be added as explicit
  carriers in later steps when facts exist or split into dependency ideas if
  semantic lowering is absent.

Added focused AArch64 dispatch coverage proving an unsupported x86-only
intrinsic spelling is not enough to fabricate an AArch64 intrinsic record, and
that removing the authoritative prepared call facts keeps the route
fail-closed with `MissingPreparedCallPlan`.

## Suggested Next

Execute Step 2 by defining the first carrier boundary for scalar FP unary
intrinsics, starting with F32/F64 `llvm.fabs` because the retained BIR call
already has typed operand/result authority. Keep F128 delegated to binary128
helper facts and leave CRC/vector/barrier/cache/hint/builtin-address families
blocked until their semantic feature and operand carriers exist.

## Watchouts

- Do not claim AArch64 intrinsic support through intrinsic-name string matching or archived scratch-register conventions.
- Keep binary128 helpers, atomics, and inline asm outside this plan.
- Unsupported x86-only intrinsics must remain rejected, trapped, or explicitly diagnosed rather than silently zero-filled.
- Ordinary `PreparedCallPlan` facts are not sufficient intrinsic authority;
  selected intrinsic support must require an explicit intrinsic carrier.
- CRC, vector, barrier/cache/hint, and builtin-address families may need
  separate semantic-lowering dependency ideas if Step 2 cannot identify
  existing structured source facts.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests. Proof log: `test_after.log`.
