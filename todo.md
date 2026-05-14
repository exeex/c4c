Status: Active
Source Idea Path: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Reconcile Target-Specific Extra Shards

# Current Packet

## Just Finished

Completed `plan.md` Step 6 ledger reconciliation for the target-specific extra
shard group: `atomics.md`, `intrinsics.md`, `inline_asm.md`, and
`peephole.md`. The real missing features were split into focused follow-up
ideas, and the stale text-first peephole optimizer was retired rather than
absorbed into AArch64 codegen as a shortcut route.

Shard: `src/backend/mir/aarch64/codegen/atomics.md`
Classification: real-missing-feature
Current Owner: frontend/preprocessor compatibility exposes atomic constants
and `_Atomic` syntax, and ordinary BIR memory records can model volatile load
or store bits, but there is no structured LIR/BIR atomic operation vocabulary,
prepared atomic ordering carrier, AArch64 atomic machine-node family, or
printer owner.
Review Result: The shard's exclusive-loop and fence semantics are still valid
AArch64 backend requirements, but the old fixed-register accumulator surface is
stale. Current compiled AArch64 codegen only names scalar, branch, memory,
spill/reload, call, return, assembler, and object instruction families, and
`MachineOpcode` has no atomic load/store/RMW/CAS/fence opcodes. Treating
atomic operations as ordinary volatile memory or as inline text would lose
ordering, failure-ordering, old-value result, exclusive-monitor, and signed
narrow-load facts. The missing feature spans structured semantic carriers plus
AArch64 selected nodes and printer support.
Proof or Evidence: `pp_predefined.cpp:281` through `:287` defines
`__ATOMIC_*` constants; `base.cpp:2805` through `:2810` recognizes `_Atomic`
as syntax, but `bir.hpp:295` through `:354` has binary/cast/select/phi
instruction vocabulary without atomic operations; `instruction.hpp:93` through
`:110` lists no atomic machine opcodes; `instruction.hpp:146` through `:155`
has memory and volatile side-effect kinds but no atomic ordering resource;
`machine_printer.cpp:359` through `:382` prints only spill/reload, branch,
memory, scalar, and return subsets.
Follow-Up: `ideas/open/238_aarch64_atomic_machine_nodes.md`.

Shard: `src/backend/mir/aarch64/codegen/intrinsics.md`
Classification: real-missing-feature
Current Owner: LIR has generic and typed intrinsic-call surfaces, BIR lowering
has runtime/intrinsic placeholder handling for variadic, stack-state, memory,
inline-asm, and limited `fabs` families, and existing open ideas cover
ordinary calls, FP nodes, memory nodes, and binary128 helper calls. There is no
compiled AArch64 target-intrinsic selected-node or printer owner for the
barrier/cache/hint, NEON/vector, CRC, builtin-address, or scalar FP unary
families described by the shard.
Review Result: The shard preserves valid target-specific families, but the old
`x0`/`w9`/`q0`/`q1` scratch-register convention and silent x86-only zero-fill
policy are stale. Current AArch64 dispatch classifies retained `bir::CallInst`
as a call family but does not lower it, and the machine printer has no call or
intrinsic printable subset. Some scalar FP and binary128 dependencies overlap
existing ideas 235 and 237, but target-specific barriers, cache hints, CRC,
builtin addresses, and vector intrinsic policy need their own structured
AArch64 intrinsic route.
Proof or Evidence: `ir.hpp:153` through `:157` defines generic LIR intrinsic
records and `ir.hpp:769` through `:778` tracks selected intrinsic requirement
flags; `calling.cpp:1167` through `:1489` lowers runtime/intrinsic placeholder
families including inline asm and `fabs`; `dispatch.cpp:91` through `:92`
classifies `bir::CallInst` as `Call`, but `dispatch.cpp:170` through `:175`
only invokes scalar lowering and reports unsupported instructions otherwise;
`machine_printer.cpp:359` through `:382` has no call or intrinsic printer
branch.
Follow-Up: `ideas/open/239_aarch64_intrinsic_machine_nodes.md`.

Shard: `src/backend/mir/aarch64/codegen/inline_asm.md`
Classification: real-missing-feature
Current Owner: LIR and BIR preserve inline-asm payload text, constraints,
argument text, return type, side-effect flags, and scalar result metadata;
prepared stack layout observes inline-asm side effects when deciding home-slot
requirements; AArch64 instruction records have an external assembler record
vocabulary. There is no compiled AArch64 owner that parses constraints,
allocates operands, substitutes template modifiers, models clobbers, or prints
selected inline-asm machine records.
Review Result: Inline assembly is a real external/textual backend feature, but
the shard's old static string substitution and helper routines are only legacy
evidence. Current BIR/prepared layers preserve enough payload and side-effect
metadata to prove the feature is not entirely absent, while AArch64 codegen
intentionally treats assembler records as deferred unsupported external input.
The missing work is a structured inline-asm contract for operands, constraints,
clobbers, homes, immediates, and template substitution, not a restoration of
the archived `asm_emitter.md` path or an independent register allocator.
Proof or Evidence: `ir.hpp:197` through `:202` defines LIR inline-asm payload
fields; `bir.hpp:365` through `:395` preserves BIR inline-asm metadata on call
instructions; `calling.cpp:1266` through `:1339` lowers LIR inline asm to a
`llvm.inline_asm` BIR call placeholder; `inline_asm.cpp:7` through `:23`
summarizes inline-asm side effects; `regalloc_helpers.cpp:42` through `:45`
uses side-effecting inline asm to keep address-exposed objects homed;
`instruction.cpp:1928` through `:1942` marks assembler records as deferred
unsupported, and `machine_printer.cpp:359` through `:382` never prints them.
Follow-Up: `ideas/open/240_aarch64_inline_asm_machine_nodes.md`.

Shard: `src/backend/mir/aarch64/codegen/peephole.md`
Classification: stale/reject-retire
Current Owner: optional post-print AArch64 text cleanup has no current compiled
owner. Shared terminal assembly output is owned by `mir::print_machine_module`
and the AArch64 `MachineInstructionPrinter`; the compatibility AArch64
assembler parser is explicitly external-input parsing, not internal codegen
semantic recovery.
Review Result: The shard describes a text-first optimizer built around the old
stack-based assembly emitter. That route conflicts with the current selected
machine-node and shared MIR printer boundary. Its local cleanup ideas may be
useful future optimization evidence, but reintroducing raw text rewrites now
would risk hiding missing lowering facts, reviving documented global
store-forwarding unsoundness, or creating testcase-shaped backend recovery.
No codegen gap should be absorbed into peephole matching.
Proof or Evidence: `machine_printer.cpp:72` through `:82` requires selected
machine nodes before printing; `machine_printer.cpp:359` through `:382` prints
only the current selected subset or returns unsupported diagnostics;
`assembler/parser.hpp:11` through `:15` says the AArch64 assembler parser is
for staged external assembler text and must not recover semantics from printed
codegen output; the shard itself records disabled global-store-forwarding
behavior around lines `72` through `74` and future ownership as optional `.s`
printer-output cleanup around lines `227` through `235`.
Follow-Up: retire note; no new idea until selected machine-node coverage is
broad enough to justify an optional, CFG-aware or node-aware optimization pass.

## Suggested Next

Execute `plan.md` Step 7 by verifying the full ledger inventory against every
source shard listed in idea 229, including the new follow-up ideas created for
target-specific atomics, intrinsics, and inline asm.

## Watchouts

- Do not mechanically convert markdown shards into same-named C++ files.
- Do not reopen the shared MIR printer boundary closed by idea 224.
- Treat `records.md` as already represented only for the narrow facts proven by
  current compiled owners; its future structured asm/object roadmap text is not
  a Step 3 implementation mandate.
- Do not recreate the stale `asm_emitter.md` inline-asm emitter, scratch
  allocator, template substitution path, or text-first assembler handoff.
- Do not classify a missing feature before checking current compiled owners.
- Memory access facts exist in BIR/prepared layers, but selected AArch64
  load/store dispatch and load printing are still missing; do not re-create the
  legacy memory scratch-register route.
- Cast record helpers are not enough: dispatch and printer support are missing
  for cast nodes.
- F32/F64 arithmetic needs typed FP/SIMD machine nodes, not integer ALU reuse.
- I128 requires prepared pair/memory carriers before AArch64 pair lowering can
  be honest.
- F128 requires full binary128 source tracking and helper-call carriers; do not
  collapse it to scalar F64 behavior.
- Global memory access support is not the same as global, label, GOT, or TLS
  address materialization; keep that with idea 233.
- Atomic lowering needs structured ordering/failure-ordering/result-mode
  carriers before AArch64 exclusive-loop machine nodes.
- Intrinsic lowering must separate target barriers/cache/CRC/vector/builtin
  families from ordinary call nodes, FP nodes, and binary128 helper routes.
- Inline asm has partial BIR/prepared metadata only; do not treat payload text
  preservation as selected AArch64 inline-asm support.
- Peephole text rewrites are retired as stale until selected machine-node
  coverage is broad enough for an optional optimization route.
- Keep compatibility projection out of terminal assembly printing; terminal
  assembly must walk shared `module::MachineModule` through
  `mir::print_machine_module` plus the AArch64 `MachineInstructionPrinter`.

## Proof

Ledger-only classification; no build or test proof required, and no broad
validation was run. No `test_after.log` was produced because the delegated
proof explicitly required ledger-only classification.

Evidence inspected with focused `rg`, `sed`, and `nl` reads of:

- `ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md`
- `src/backend/mir/aarch64/codegen/atomics.md`
- `src/backend/mir/aarch64/codegen/intrinsics.md`
- `src/backend/mir/aarch64/codegen/inline_asm.md`
- `src/backend/mir/aarch64/codegen/peephole.md`
- `src/codegen/lir/ir.hpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` / `.hpp`
- `src/backend/mir/aarch64/assembler/parser.hpp`
- `src/backend/mir/aarch64/assembler/types.hpp`
- `src/backend/prealloc/stack_layout/inline_asm.cpp`
- `src/backend/prealloc/stack_layout/regalloc_helpers.cpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/frontend/preprocessor/pp_predefined.cpp`
- `src/frontend/parser/impl/types/base.cpp`
- `ideas/open/231_aarch64_call_frame_machine_nodes.md`
- `ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md`
- `ideas/open/237_aarch64_binary128_softfloat_lowering.md`
- `ideas/open/238_aarch64_atomic_machine_nodes.md`
- `ideas/open/239_aarch64_intrinsic_machine_nodes.md`
- `ideas/open/240_aarch64_inline_asm_machine_nodes.md`
