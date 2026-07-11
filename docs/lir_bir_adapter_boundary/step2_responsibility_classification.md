# Step 2 Responsibility Classification

Source idea: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
Plan step: Step 2 - Classify Responsibilities By First Owning Layer

This is a docs-only classification of the `LIR -> BIR` adapter responsibilities
inventoried in
`docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`. No
implementation files, tests, expectations, unsupported markers, allowlists,
or tracked build artifacts were changed.

## Evidence Sources

Primary inventory input:

- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`

Comparison evidence:

- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

Transient scan paths cited by those docs remain evidence pointers only:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

## First Owning Layer Summary

| Exposed responsibility | First owning layer | Current surface | Classification decision |
| --- | --- | --- | --- |
| Public lowering API, options, notes, and diagnostics | LIR import | `lir_to_bir.hpp`, `lir_adapter_error.hpp`, root `lir_to_bir.cpp`, `context.cpp`, `analysis.cpp`, `module.cpp` | Adapter front door. Keep the result envelope, options, unsupported/malformed diagnostics, prescan facts, and notes in the import layer. |
| Target profile selection that changes import behavior | LIR import | `BirLoweringOptions`, `BirLoweringContext`, `module.cpp`, backend callers | Import-local when it controls whether LIR can be admitted to semantic BIR. Target emission policy remains downstream. |
| Module prescan, instruction counting, and lowering note construction | LIR import | `analysis.cpp`, `context.cpp`, `module.cpp` | Adapter-owned import bookkeeping; not canonical BIR model data. |
| Scalar and CFG conversion from LIR instructions and blocks | LIR import | `scalar.cpp`, `cfg.cpp`, `module.cpp` | Adapter-owned semantic production. Output must be canonical BIR values, blocks, and terminators, but the raw LIR spelling maps stay private to import. |
| Legacy type text parsing, type declaration lookup, and typed operand parsing | Structured type/layout bridge | `types.cpp`, `lowering.hpp`, `TypeDeclMap`, `GlobalTypes` type text | Bridge layer between producer spellings and BIR type facts. Keep compatibility with legacy LIR text inside the adapter until a narrower typed contract is proven. |
| Structured layout lookup and aggregate layout fallback | Structured type/layout bridge | `BackendStructuredLayoutTable`, `BackendStructuredLayoutEntry`, `BackendAggregateLayoutLookup`, `aggregate.cpp`, `types.cpp`, `memory_helpers.hpp` | Adapter bridge when translating LIR aggregate spellings and structured declarations. Canonical structured type lookup remains BIR/module model authority. |
| Pure projection/layout helper declarations used by memory lowering | Structured type/layout bridge | `memory/memory_helpers.hpp` | Valid narrow private helper owner. Do not widen it into stateful lowerer policy or public BIR memory authority. |
| Global declarations, link-name resolution, known global addresses, and runtime element shape import | Initializer bridge | `globals.cpp`, `global_initializers.cpp`, `GlobalTypes`, `FunctionSymbolSet` | Adapter-owned importer from LIR/global spellings to semantic BIR globals, link ids, string constants, and known address facts. |
| Scalar, byte string, array, aggregate, and pointer initializer lowering | Initializer bridge | `global_initializers.cpp`, root string constant rewrite helpers, `value_materialization.cpp` | Adapter-owned initializer bridge. It should output semantic BIR/global data without making textual initializer compatibility a BIR model contract. |
| Local slots, allocas, load/store, GEP, pointer arrays, pointer-address records, and intrinsic memory import | Memory/address provenance import | `memory/*.cpp`, `memory_types.hpp`, `memory_helpers.hpp` | Adapter-owned conversion of LIR memory and pointer provenance into BIR memory/address records plus import side tables. Public BIR memory authority remains outside the private adapter tree. |
| Same-module formal pointer provenance publication during lowering | Memory/address provenance import | `module.cpp`, memory provenance files | Adapter-owned imported provenance fact. It should remain behavior-preserving and not become prepared destination or target storage policy. |
| Direct call, return, byval, vararg, HFA, and signature ABI metadata admitted from LIR | Call ABI import | `call_abi.cpp`, `calling.cpp`, `module.cpp` | Adapter-owned semantic call/return ABI import. Physical registers, outgoing stack layout, aggregate transport lanes, and scratch policy are prepared/target owners. |
| Inline asm and runtime call/intrinsic admission from LIR into BIR | Call ABI import | `calling.cpp`, `memory/intrinsics.cpp` | Import-owned when translating LIR operation semantics and call metadata. Target instruction selection, helper/carrier policy, and final emission stay downstream. |
| `bir::Module`, `Function`, `Block`, `Inst`, `Value`, names, ids, terminators, memory address payloads, route records, printer, validator, and public queries | Canonical BIR semantic model | `bir.hpp`, `bir.cpp`, `bir_printer.cpp`, `bir_validate.cpp` | BIR-owned output model and query authority. Do not move these public records into `lir_to_bir/` unless a later packet proves they are lowering-only. |
| BIR route annotations and query indexes for producer, select-chain, memory, publication, edge/join, call-use, comparison, and facade records | Canonical BIR semantic model | Route records and query helpers documented by BIR/prealloc fusion | BIR-owned target-neutral relationship model. The adapter may populate or consume semantic BIR facts, but must not own the route schemas as import compatibility state. |
| Prepared module assembly, lookup bundles, frame/dynamic stack/call/storage/object-data plans, carriers, liveness, regalloc, value homes, and MIR-facing publication | Prepared/prealloc handoff | `prealloc.cpp`, `prealloc/module.hpp`, prepared docs | Downstream handoff after adapter success. Retain prepared aggregate and lookup delivery until narrow agreement-gated migrations prove otherwise. |
| RV64, AArch64, x86 MIR/object lowering and runtime comparison outcomes | Prepared/prealloc handoff or target consumer | backend target routes, RV64 post-contract docs | Consumer evidence, not adapter ownership evidence, unless logs stop at `semantic lir_to_bir` or bootstrap/global data handoff before prepared object generation. |

## Import-Local Compatibility Maps

The following maps and scratch structures should be treated as import-local
compatibility state. A future cleanup may hide or split them behind narrower
adapter contracts, but Step 2 finds no evidence that they should become
canonical BIR, prepared, or target public surfaces.

| Compatibility surface | First owning layer | Reason to keep import-local |
| --- | --- | --- |
| `ValueMap` | LIR import | Keys are LIR SSA spellings mapped to newly lowered BIR values. The key space is producer text, not BIR identity. |
| `GlobalTypes` | Initializer bridge | Carries type text, link ids, initializer metadata, pointer initializer offsets, runtime element shape, and known global addresses while importing LIR globals. |
| `TypeDeclMap` | Structured type/layout bridge | Records legacy producer type declaration text. Canonical BIR should see structured type facts, not raw compatibility spellings. |
| `FunctionSymbolSet` | Initializer bridge | Keeps authoritative link-name ids plus raw-symbol fallback for textual pointer initializer compatibility. |
| `LocalSlotTypes`, `LocalPointerSlots`, `LocalIndirectPointerSlotSet` | Memory/address provenance import | Function-local spelling/provenance side tables used while translating allocas, pointer slots, and indirect pointer facts. |
| `BackendStructuredLayoutTable`, `BackendStructuredLayoutEntry`, `BackendAggregateLayoutLookup` | Structured type/layout bridge | Adapter bridge for structured layout and legacy fallback parity. BIR/module structured type lookup remains the model authority. |
| `CompareMap` | LIR import | Temporary scalar/compare lowering scratch keyed by local lowering context. BIR Route 7 comparison records are the canonical semantic relationship owner. |
| `BlockLookup` | LIR import | Maps producer block spellings to BIR block ids during CFG import. BIR block labels/ids are the durable model. |
| `AggregateValueAliasMap`, `AggregateParamMap` | Structured type/layout bridge | Aggregate import aliasing and byval parameter bridge state, not target aggregate transport policy. |
| `PhiBlockPlanMap`, `PendingAggregatePhiCopyMap`, `PendingScalarPhiProducerMap` | LIR import | CFG/phi construction scratch while building BIR blocks. Prepared edge/join publication and BIR Route 5 identities remain separate downstream/model concerns. |
| Memory side tables from `memory_types.hpp` | Memory/address provenance import | Pointer-address, dynamic array, local aggregate, and value materialization maps are adapter state for producing semantic BIR memory/address facts. |

## Comparison Against BIR Core Cleanup

The BIR cleanup docs support a strict separation between the private adapter
and the public semantic model:

- `docs/bir_core_cleanup/implementation_inventory.md` classifies
  `lir_to_bir/` as an existing private lowering subsystem with functional
  owners for analysis, types, globals, initializers, aggregate ABI, call ABI,
  scalar, CFG, calling, context, module orchestration, and memory/provenance
  lowering.
- `docs/bir_core_cleanup/destination_map.md` says to treat `lir_to_bir/` as
  private lowering and not move public BIR model records into that tree unless
  they are proven lowering-only.
- The same destination map keeps `Value`, `Inst`, `Block`, `Function`,
  `Module`, `MemoryAddress`, route-index facade surfaces, and route records in
  central BIR/model ownership during early cleanup.
- `memory_helpers.hpp` is explicitly a narrow private helper precedent for pure
  layout/projection helpers, not a destination for stateful policy or public
  memory authority.

This Step 2 split agrees with those docs. It classifies adapter type/layout,
initializer, memory, and call responsibilities as import or bridge work, while
leaving public BIR route records, memory authority, route query APIs, printer,
validator, and model containers in canonical BIR ownership.

## Comparison Against BIR/Prealloc Fusion

The BIR/prealloc fusion docs distinguish target-neutral semantic relationship
facts from prepared target/layout products:

- `phase_a_normalization_candidates.md` accepts same-block producer,
  select-chain, memory/access identity, current-block publication, CFG
  edge/join identity, call-boundary semantic source facts, and comparison
  producer identity as BIR-normalization candidates.
- The same Phase A artifact rejects homes, frame slots, stack offsets, physical
  registers, ABI placement, target addressing legality, relocation spelling,
  storage hooks, move scheduling, branch emission, scratch resources, helper
  protocols, final instruction records, and whole mixed `Prepared*Plan` shapes
  from canonical BIR.
- `phase_c_private_cache_contraction.md` records that implemented BIR route
  indexes can replace selected semantic reads only after consumer-by-consumer
  equivalence, while prepared helpers remain migration oracles and cache
  surfaces until residual consumers move.
- `phase_e5_prepared_bir_module_demotion_or_retirement_gate.md` keeps
  `PreparedBirModule` and `PreparedFunctionLookups` aggregate delivery
  retained. It allows only narrow future adapters that prove one semantic read
  and preserve prepared fallback, diagnostics, wrapper output, and expected
  strings.

This Step 2 classification therefore avoids two collapse errors:

1. It does not make the LIR importer the owner of BIR route schemas just
   because the adapter produces `bir::Module`.
2. It does not make prepared/prealloc the owner of LIR import compatibility
   maps just because prepared consumers need semantic BIR after import.

The adapter boundary should hand off canonical BIR semantics to BIR and then
prepared/prealloc should derive target/layout products from that model.

## Comparison Against RV64 Post-Contract Evidence

The RV64 post-contract evidence is useful for first-owner routing, but it does
not by itself widen adapter ownership:

- `bir_semantic_admission_classification.md` classifies `373` exact semantic
  rows that stop at `semantic lir_to_bir` before prepared object handoff:
  `264` local-memory facts, `55` call metadata rows, `34` runtime/intrinsic
  memory rows, and `20` scalar/signature/control rows. Those are adapter/BIR
  producer admission evidence, not RV64 or prepared publication evidence.
- The same document keeps `44` bootstrap/global data-shape handoff rows
  separate from the exact semantic rows. Those rows support initializer/global
  bridge follow-up work, not broad prepared or RV64 ownership.
- `failure_bucket_map.md` routes explicit prepared/module-shape diagnostics to
  prepared/BIR boundary review before RV64 consumption, routes coherent
  unsupported RV64 object lowering to RV64 only when BIR/prepared facts are
  available, and treats rows without explicit diagnostics as evidence gaps.

Applied to the adapter boundary, the RV64 evidence supports these conclusions:

- Local-memory, GEP, load/store, alloca, scalar/local-memory, memcpy, and
  memset semantic failures are first owned by memory/address provenance import
  or canonical BIR semantic producer facts, depending on whether the missing
  fact is an import admission rule or a target-neutral BIR relationship.
- Direct-call and call-return semantic failures are first owned by call ABI
  import or canonical BIR call-use/source semantics, not by RV64 emission.
- Scalar, signature, and control failures are LIR import or canonical BIR
  model admission issues until a later log proves prepared or target ownership.
- Prepared move-bundle, local-memory-access, global-data, stack-frame, and
  publication diagnostics are downstream boundary evidence. They must not be
  repaired by guessing missing prepared facts in RV64 or by moving prepared
  policy into the adapter.

## Boundary Decisions For Follow-Up Work

- Behavior-preserving adapter cleanup should start by hiding or narrowing
  import-local compatibility maps and private declarations, not by moving
  public BIR model records into `lir_to_bir/`.
- Structured layout and initializer bridge follow-ups should preserve legacy
  spelling compatibility while moving toward narrower typed adapter contracts.
- Memory/address provenance follow-ups must distinguish imported pointer/local
  facts from BIR Route 3 memory-access identity and from prepared/target frame
  or addressing policy.
- Call ABI import follow-ups must distinguish semantic call/return metadata
  from prepared call plans, ABI placement, byval transport, outgoing stack
  layout, and target wrapper behavior.
- Prepared/prealloc handoff follow-ups should consume canonical BIR or route
  facts through agreement-gated adapters; they should not absorb LIR import
  compatibility maps or retire prepared aggregates wholesale.
- RV64 follow-ups should act only when the row evidence shows coherent
  BIR/prepared facts rejected by RV64 lowering. Rows that fail before prepared
  handoff remain adapter/BIR admission work.

## Step 2 Handoff

The durable ownership vocabulary for later packets is:

1. `LIR import`: public adapter entry, diagnostics, module prescan, scalar/CFG
   conversion, and raw producer spelling maps.
2. `Structured type/layout bridge`: legacy type text, structured layout
   fallback, aggregate layout lookup, typed operand parsing, and pure
   projection helpers.
3. `Initializer bridge`: global declaration import, string constants, scalar
   and aggregate initializers, pointer initializers, link ids, and known global
   addresses.
4. `Memory/address provenance import`: local slots, pointer slots, GEPs,
   local/global loads and stores, intrinsics, dynamic arrays, pointer-address
   records, and formal pointer provenance imported from LIR.
5. `Call ABI import`: signature, argument, return, byval, vararg, HFA, inline
   asm, and runtime call metadata admitted from LIR into semantic BIR.
6. `Canonical BIR semantic model`: public model containers, instruction
   payloads, memory address records, route records, route indexes, public
   query APIs, printer, and validator.
7. `Prepared/prealloc handoff`: `PreparedBirModule`,
   `PreparedFunctionLookups`, value homes, frame/stack/call/storage/object
   plans, carriers, liveness, regalloc, publication policy, MIR consumers, and
   target object/runtime routes.

Later Step 3 documents should use this vocabulary consistently and keep the
import-local maps out of canonical BIR, prepared/prealloc policy, and RV64
target ownership unless a narrower follow-up proves a different boundary.
