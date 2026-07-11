Status: Active
Source Idea Path: ideas/open/688_initializer_lowering_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Initializer Bridge Surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for the initializer lowering bridge surfaces.

Inventory summary:
- `src/backend/bir/lir_to_bir/global_initializers.cpp` is mostly already
  adapter-private: scalar parsing, LLVM byte-string lowering, integer-array
  lowering, aggregate recursive materialization, zero-fill expansion, and GEP
  initializer parsing are local helpers. The exported helpers are the bridge
  entry points declared in `lowering.hpp`: `parse_global_address_initializer`,
  `lower_global_initializer`, `lower_integer_array_initializer`,
  `strip_typed_initializer_prefix`, and aggregate initializer lowerers.
- `src/backend/bir/lir_to_bir/globals.cpp` owns global declaration lowering,
  string-pool global lowering, `FunctionSymbolSet`, pointer-initializer offset
  resolution, and known global-address resolution. It is the right first
  surface for behavior-preserving bridge isolation because the helpers already
  translate LIR spelling into `GlobalInfo`, `GlobalAddress`, `bir::Global`, and
  `LinkNameId` facts before prepared object-data or target emission.
- `GlobalTypes` and `GlobalInfo` are adapter-private in namespace and header
  placement, but too wide inside the adapter: memory/provenance, local-slot,
  addressing, calling, and module paths inspect raw-name keyed compatibility
  fields such as `type_text`, `initializer_symbol_name`,
  `known_global_address`, and `pointer_initializer_offsets`.
- `FunctionSymbolSet` is adapter-private in namespace and header placement,
  but too wide inside the adapter because its raw-symbol lookup is visible to
  general memory/provenance and local-slot lowering. Existing comments already
  identify it as the no-id compatibility bridge for imported function pointer
  initializers.
- String-pool global lowering is adapter-owned, but string constant target-id
  rewrite currently spans `lower_string_constant_global()` in `globals.cpp`,
  module-level `apply_string_pointer_initializer_target_ids()`, and later
  pointer-initializer value-id publication. That is a good later packet, not
  the first one, because it reaches relocation-slot publication.
- Known global-address resolution is adapter bridge work, but it is shared by
  module pre-pass resolution and memory/provenance consumers. Moving it first
  would risk widening into memory/provenance cleanup rather than isolating the
  initializer bridge.

## Suggested Next

First implementation packet: narrow the global declaration/function-symbol
import bridge surface without changing behavior.

Exact owned files/helpers for the next packet:
- `src/backend/bir/lir_to_bir/lowering.hpp`: replace the generic
  `FunctionSymbolSet` name at the declaration boundary with an
  adapter-specific name such as `InitializerFunctionSymbolSet` or
  `ImportedFunctionSymbolIndex`, preserving the same methods and storage.
- `src/backend/bir/lir_to_bir/globals.cpp`: update the type/method
  definitions and helper signatures for
  `is_known_raw_function_symbol()`,
  `is_known_function_global_address()`,
  `resolve_known_global_address()`, and
  `resolve_pointer_initializer_offsets()` without changing lookup rules.
- `src/backend/bir/lir_to_bir/module.cpp`: update construction/population at
  the module boundary and existing call sites to the renamed bridge type only.

Why this is initializer bridge work: the packet isolates the raw/no-id imported
function lookup used to resolve pointer global initializers, aggregate pointer
fields, and known global-address aliases into `LinkNameId`-backed BIR facts.
It does not alter prepared object-data plans, target relocation spelling, MIR
consumers, memory/provenance semantics, call ABI, or runtime behavior.

## Watchouts

- Keep the first packet mechanical: no helper extraction that changes lookup
  order, no fallback changes, no new public BIR fields, no expectation edits,
  and no testcase-shaped special cases.
- Do not make `GlobalTypes` private in the first packet. It is widely consumed
  by memory/addressing/provenance code and needs a separate seam after the
  declaration/function-symbol bridge is named clearly.
- Do not start with string constant rewrite state. It touches module-level
  relocation-slot target id publication, so it is a broader Step 3 packet.
- `ctest --test-dir build -N` did not reveal a dedicated global-initializer
  subset narrower than the backend suite; keep the supervisor-provided backend
  proof for the next code-changing packet.

## Proof

No build or test proof run; this packet was inventory-only per delegation.

Expected proof command for the next implementation packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

No `test_after.log` was written for this inventory-only packet.
