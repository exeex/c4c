Status: Active
Source Idea Path: ideas/open/268_aarch64_intrinsics_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Intrinsics Shard And Current Route

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: audited
`src/backend/mir/aarch64/codegen/intrinsics.md` against the current compiled
AArch64 backend route.

Current intrinsic facts worth preserving:

- Prepared intrinsic carriers are populated for BIR calls with intrinsic
  metadata and carry family, operation, required feature, operand/result types,
  operand roles, vector shape, signedness, memory access, barrier domain,
  immediate facts, side-effect status, value homes, result home, source callee,
  and prepared call-plan presence.
- The compiled codegen route already dispatches calls with prepared intrinsic
  carriers before ordinary calls.
- Selected intrinsic machine nodes currently exist for:
  `ScalarFpUnary/FAbs` on matching `F32` or `F64` operands/results,
  side-effect-free and feature-free; `Crc/Crc32W` with `AArch64Crc`, unsigned
  `I32` accumulator/data/result roles; `VectorMemory/VectorLoad` with
  `AArch64Neon`, `Ptr -> I128`, read-only default-address-space `v16i8`
  memory facts; and `VectorOperation/VectorAdd` with `AArch64Neon`, unsigned
  `v16i8` `I128` lhs/rhs/result facts.
- Current printer spelling is structured-machine-node based: `fabs`,
  `crc32w`, `ld1 {vN.16b}, [xM]`, and `add vD.16b, vN.16b, vM.16b`, with
  fail-closed checks for complete prepared carrier provenance and register
  authority.

Unsupported or deferred families to keep fail-closed:

- Prepared carrier validation recognizes but current AArch64 codegen does not
  lower `Barrier/BarrierDmb`, `CacheMaintenance/CacheDcCvau`, or
  `PauseHint/HintYield`; these should remain explicit unsupported/deferred
  routes until Step 2 gives them selected machine-node and printer ownership.
- Any incomplete carrier, unsupported family, unsupported intrinsic operation,
  missing prepared call plan, missing value home, unsupported storage, missing
  feature/role/vector/memory facts, or non-selected machine-node status must
  keep producing diagnostics instead of placeholder assembly.

Obsolete reference-only material to reject from `intrinsics.md`:

- Do not port the old `ArmCodegen` helper surface (`emit_neon_binary_128`,
  `store_scalar_dest`, scalar raw-bit `fmov` helper glue,
  `emit_nontemporal_store`, or destination-stack-slot helper patterns).
- Do not resurrect the broad legacy mappings for fences as `dmb ish/ishst`,
  `Pause` as `yield`, `Clflush` as `dc civac`, ordinary non-temporal stores,
  unaligned 128-bit load/store placeholders, SSE-equivalent NEON operations
  beyond current `VectorAdd`, CRC widths other than current `Crc32W`, address
  builtins (`FrameAddress`, `ReturnAddress`, `ThreadPointer`), `SqrtF32/F64`,
  x86-only zero-fill behavior, or binary128 soft-float helper commentary.
- Treat the register-convention and hidden-assumption sections as historical
  risk notes only; current ownership must consume prepared storage/allocation
  facts and structured machine records, not fixed local scratch conventions.

## Suggested Next

Execute Step 2 from `plan.md`: create `intrinsics.cpp` and `intrinsics.hpp` as
the AArch64 intrinsic owner boundary, moving the current carrier lookup,
record construction, selected-status checks, and printer helpers out of
`dispatch.cpp`, `instruction.cpp`, and `machine_printer.cpp` without expanding
the accepted intrinsic surface.

## Watchouts

- Keep this route behavior-preserving unless current prepared facts and tests
  already authorize the behavior.
- Do not port the legacy NEON, CRC, cache, hint, address builtin, or soft-float
  helper surface wholesale.
- Do not mix scalar float, F128, memory, call, return, allocation, assembler,
  or unrelated printer rewrites into this shard redistribution.
- The concrete routing/owner issue for Step 2 is that intrinsic ownership is
  split across broad owners today: `dispatch.cpp` finds carriers and constructs
  CRC/vector records, `instruction.cpp` owns intrinsic records and selection
  status, and `machine_printer.cpp` owns final intrinsic spelling. Step 2
  should centralize only this current accepted/deferred boundary in the new
  intrinsic owner.

## Proof

No build required for this audit-only packet. `test_after.log` was not touched
because the delegated packet explicitly marked proof as not required and listed
`test_after.log` under Do Not Touch.
