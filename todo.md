Status: Active
Source Idea Path: ideas/open/685_lir_import_context_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Import Context Baseline

# Current Packet

## Just Finished

Completed `plan.md` Step 1 baseline inventory for the LIR import context.

Public adapter entry behavior:
- `BirLoweringOptions` is the caller-controlled import configuration: CFG, phi,
  memory, calls, type legalization, aggregate lowering, dynamic alloca
  preservation, and bounded pattern folds.
- `try_lower_to_bir_with_options` builds a private `BirLoweringContext`, records
  pipeline notes, runs `analyze_module`, runs `lower_module`, optionally applies
  the string-pointer rewrite to a successful module, and returns the result
  envelope.
- `BirLoweringResult` preserves the public envelope as optional `bir::Module`,
  `BirModuleAnalysis`, and ordered lowering notes.
- `try_lower_to_bir` is the default-options optional-module convenience entry.
- `lower_to_bir` is the throwing convenience entry; it returns the module on
  success and throws `std::invalid_argument` on failed optional lowering.
- `LirAdapterError` carries unsupported versus malformed adapter diagnostics
  with `unsupported`, `malformed`, and `is_unsupported`; this packet found no
  need to change that public diagnostic shape.

Private import-context state inventory:
- `BirLoweringContext` owns private import input state: source `LirModule`,
  selected `TargetProfile`, copied `BirLoweringOptions`, and accumulated notes.
- `analysis.cpp` owns module/function prescan facts and emits the `pre_scan`
  note into the context.
- `module.cpp` owns route-local orchestration for global/type/function imports:
  `GlobalTypes`, `FunctionSymbolSet`, `TypeDeclMap`, structured-layout tables,
  string constants, same-module formal pointer provenance, target ABI pressure
  adjustments, function lowering dispatch, and failure-note ranking.
- `lowering.hpp` exposes broad private split-TU state through
  `lir_to_bir_detail` and `BirFunctionLowerer`, including `ValueMap`,
  `CompareMap`, `BlockLookup`, `PhiBlockPlanMap`,
  `PendingAggregatePhiCopyMap`, `PendingScalarPhiProducerMap`, raw global/type
  spelling maps, aggregate alias/parameter maps, memory side tables, and local
  value/provenance caches.
- The most adapter-local first state family is CFG/phi scratch:
  `BlockLookup`, `BranchChain`, `PhiLoweringPlan`, `PhiBlockPlanMap`,
  `PendingAggregatePhiCopyMap`, and `PendingScalarPhiProducerMap`. These are
  keyed by function-local LIR block/value spellings, are consumed while building
  BIR blocks, and are not public BIR, prepared/prealloc, target, test, or
  expectation state.

## Suggested Next

First implementation packet: narrow CFG/phi scratch ownership without changing
behavior.

Owned files for the packet should be limited to:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `todo.md`

Suggested implementation shape: move or group the CFG/phi scratch declarations
that are only needed by `BirFunctionLowerer` and `module.cpp` behind a smaller
adapter-private boundary, preferably by making the helper structs/aliases local
to `module.cpp` or nested tightly under `BirFunctionLowerer` only where method
signatures still require them. Do not touch public adapter entries,
structured-layout bridge helpers, initializer/global maps, memory side tables,
call ABI helpers, tests, expectations, unsupported markers, allowlists, or
docs. This is adapter-local and behavior-preserving because it changes only
private declaration visibility for function-local CFG/phi construction scratch;
the emitted `BirLoweringResult`, notes, diagnostics, optional result behavior,
and BIR module contents should be unchanged.

## Watchouts

Keep this route clear of the later ordered ideas: structured layout, initializer,
memory/provenance, and call ABI state are visible in the same broad header but
should not be folded into this first packet. `CompareMap` and `ValueMap` are
shared across scalar/select and memory paths, so they are higher-risk first
targets than the CFG/phi plan maps. If the selected CFG/phi declarations turn
out to be required across multiple split translation units, stop and report the
specific cross-TU dependency instead of widening the packet.

## Proof

Delegated proof passed: `git diff --check -- todo.md`.

No `test_after.log` was produced because the delegated proof is a diff hygiene
check for the `todo.md` inventory-only packet, not a build or test command.
