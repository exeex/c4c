Status: Active
Source Idea Path: ideas/open/686_private_detail_header_contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Private Detail Header Responsibilities

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory for
`src/backend/bir/lir_to_bir/lowering.hpp`.

Inventory method:
- Read the active source idea and the runbook `Read First` handoff docs.
- Confirmed `c4c-clang-tool` and `c4c-clang-tool-ccdb` are available on
  `PATH`.
- Used `c4c-clang-tool function-signatures` and `list-symbols` on
  `lowering.hpp`, then targeted `rg` reference checks across
  `src/backend/bir/lir_to_bir/`.

Declaration-family classification:
- Public/private adapter orchestration: `BirLoweringContext`,
  `make_lowering_context`, `analyze_module`, and `lower_module`.
  Classification: several adapter TUs plus root adapter entry path; adapter
  private implementation contract, not a non-adapter/public BIR model surface.
- Import-local global and initializer state: `GlobalAddress`, `GlobalInfo`,
  `GlobalTypes`, `FunctionSymbolSet`, and global/initializer helper
  declarations. Classification: several adapter TUs, especially `module.cpp`,
  `globals.cpp`, `global_initializers.cpp`, and memory provenance/local-slot
  TUs; adapter-private initializer bridge. `FunctionSymbolSet` state crosses
  TUs, but its inline method bodies are exposed wider than necessary.
- Structured type/layout bridge: `TypeDeclMap`, `AggregateField`,
  `AggregateTypeLayout`, `BackendStructuredLayoutEntry`,
  `BackendStructuredLayoutTable`, `BackendAggregateLayoutLookup`, and layout
  lookup/build helpers. Classification: several adapter TUs including
  `types.cpp`, `aggregate.cpp`, `call_abi.cpp`, `calling.cpp`, and memory TUs;
  adapter-private structured layout bridge. Not a public BIR type/model
  surface.
- Typed operand and index parsing helpers: `ParsedTypedOperand`,
  `parse_typed_operand`, and `resolve_index_operand`. Classification: several
  adapter TUs, especially `types.cpp`, `module.cpp`, `calling.cpp`, and
  memory addressing/GEP/local-slot files; adapter-private LIR spelling bridge.
- Scalar type/size helpers: `parse_i64`, `lower_integer_type`, and
  `type_size_bytes`. Classification: broad cross-adapter helper family used by
  scalar, module, call, globals, aggregate, and memory TUs; adapter-private,
  but not a narrow first packet.
- Call ABI helpers: `compute_call_arg_abi`, `compute_function_return_abi`,
  call signature parse records, `LoweredReturnInfo`, HFA return lane records,
  and call/declaration lowering entry points. Classification: cross-adapter
  call ABI import surface; adapter-private, but later ordered call ABI cleanup
  should own semantic isolation.
- `BirFunctionLowerer` stateful method surface and aliases imported from
  `memory/memory_types.hpp`. Classification: broad split-TU adapter state used
  by scalar, CFG, aggregate, calling, module, and memory files. This remains
  private LIR import state and should not move into public BIR, prepared, MIR,
  or target ownership.
- CFG/phi scratch: `BlockLookup`, `BranchChain`, `PhiBlockPlanMap`,
  `PendingAggregatePhiCopyMap`, and `PendingScalarPhiProducerMap`.
  Classification: mostly `cfg.cpp` plus `module.cpp` and `BirFunctionLowerer`
  fields; adapter-private LIR import scratch.
- Memory/provenance side-table aliases and memory method declarations:
  `LocalSlotTypes`, `LocalPointerSlots`, `LocalIndirectPointerSlotSet`, and
  aliases from `memory_types.hpp`. Classification: broad memory adapter
  TUs plus stateful `BirFunctionLowerer`; adapter-private memory/address
  provenance import. Not a public BIR memory authority surface.
- Non-adapter use check: exact header path is under
  `src/backend/bir/lir_to_bir/`; basename `lowering.hpp` also exists in other
  subsystems, so raw include hits outside this directory are not evidence that
  this private detail header is consumed by non-adapter code. No non-adapter
  ownership for the declarations above was found.

## Suggested Next

First implementation packet: contract `FunctionSymbolSet` inline method bodies
out of `src/backend/bir/lir_to_bir/lowering.hpp`.

Owned files:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `todo.md`

Packet shape:
- Keep the `FunctionSymbolSet` type and method declarations in `lowering.hpp`
  because the state is genuinely cross-TU adapter-private input to module,
  global, call, scalar, and memory/provenance lowering.
- Move the four inline method definitions (`reserve`, `insert_function`,
  `contains_link_name_id`, `find_raw_symbol_link_name_id`) into `globals.cpp`
  near the other function-symbol/global-address helpers.
- Do not change fields, lookup semantics, link-name fallback behavior, global
  initializer resolution, unsupported markers, tests, expectations, or public
  APIs.

Why this is the narrow first packet:
- It reduces detail-header implementation width without changing the required
  cross-TU declaration surface.
- The family is initializer/import-private, keyed by adapter raw-symbol and
  `LinkNameId` compatibility state.
- `globals.cpp` already owns `is_known_function_*`,
  `resolve_known_global_address`, and pointer-initializer resolution helpers
  that consume `FunctionSymbolSet`.
- The change is behavior-preserving: out-of-line method bodies should preserve
  the same calls and data members while shrinking what every split adapter TU
  sees in the header.

## Watchouts

The next packet should compile the affected adapter TUs because removing inline
bodies can expose missing includes or link errors. Keep `FunctionSymbolSet`
state itself private to LIR import; do not move the type into public BIR,
prepared/prealloc, target, MIR, or a broad new helper bucket. Avoid structured
layout, initializer semantics, memory/provenance repair, call ABI repair, test
expectation changes, unsupported marker changes, or allowlist edits.

## Proof

Ran `git diff --check -- todo.md` for this Step 1 inventory-only packet. The
delegated proof is diff-check only and does not produce `test_after.log`; no
root-level proof logs were created or modified.
