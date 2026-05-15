Status: Active
Source Idea Path: ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Semantic Intrinsic Facts

# Current Packet

## Just Finished

Plan Step 2 - Define Semantic Intrinsic Facts completed for one no-result
AArch64 barrier representative.

Implemented semantic authority:

- Added BIR `Barrier` intrinsic family and `BarrierDmb` operation facts.
- Added a structured `sy` barrier-domain carrier plus stored immediate value
  authority for required-immediate intrinsic operands.
- Lowered only `llvm.aarch64.dmb(i32 15)` on AArch64 into a no-result,
  side-effecting BIR intrinsic call with a `BarrierDomain` operand role.
- Kept unsupported DMB domains, non-immediate domains, target mismatches, and
  existing x86/CRC/vector malformed neighbors fail-closed under the AArch64
  semantic intrinsic diagnostic family.
- Prepared carriers and AArch64 selected-machine dispatch still do not claim
  support for barrier intrinsics.

## Suggested Next

Proceed to `plan.md` Step 3 for the same barrier representative: publish a
prepared intrinsic carrier for complete `BarrierDmb` semantic facts while
keeping selected-machine dispatch closed.

## Watchouts

- The accepted representative is deliberately narrow: DMB `sy` is the only
  admitted barrier domain so far; nearby DMB spellings remain diagnostic-only.
- `src/backend/bir/lir_to_bir/calling.cpp` was required for the semantic
  intrinsic recognizer because LLVM direct intrinsics do not lower through the
  generic direct-call path in `src/backend/bir/lir_to_bir.cpp`.
- Step 3 must handle void-result prepared validation explicitly; existing
  complete prepared intrinsic validators still assume result homes for scalar,
  CRC, and vector carriers.
- Do not add selected AArch64 machine records or printer spellings in the
  prepared-carrier packet.

## Proof

Delegated proof for the Step 2 semantic intrinsic packet:

- `set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|prepared_printer|aarch64_instruction_dispatch)'; } 2>&1 | tee test_after.log`
- Result: passed, 3/3 tests green.
- Proof log: `test_after.log`
