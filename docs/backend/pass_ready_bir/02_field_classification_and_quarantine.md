# Field Classification and Legacy Quarantine

This document assigns every field and emitted family inventoried in
`01_current_lir_to_bir_schema.md` exactly one destination. The assignment is a
design decision; current locations and consumers remain the confirmed facts
cited by Step 1. A field must have one registry row below, and a row has exactly
one class code.

## Destination classes and rules

| Code | Destination | Authority and lifetime |
|---|---|---|
| C | future core IR | Sole semantic authority from raw-BIR construction through canonical BIR. Owned by stable module/function storage and changed only through verified mutation APIs. Display rendering is derived from IDs, never competing authority. |
| A | recomputable analysis | Never serialized as semantic truth. Built from a canonical IR revision, cached only with that revision, invalidated by declared mutations, and freely discarded/rebuilt. Dense indices and pointers may exist only inside the result lifetime. |
| L | lowering-only input | Adapter-private evidence used while creating raw BIR. It dies at raw-BIR publication and cannot be read by canonical passes, preparation, or MIR. |
| P | prepared/MIR output | Target/ABI/allocation/frame/instruction-selection policy created only after verified canonical BIR. It belongs to a typed prepared or MIR product, not core BIR, and dies with that product. |
| X | legacy compatibility/debug | Observational duplicate, spelling, or old route fact held only in `LegacyBirCompatibilityCapsule`. It is non-authoritative, read-only after raw construction, inaccessible to core passes, and must reach zero fields/readers. |

There is no fallback class. When migration temporarily needs both a C field and
an X rendering of it, the X value is a derived snapshot; the C value remains the
only authority.

## Exhaustive classification registry

Rows partition the Step 1 inventory: brace lists are the complete fields in that
row, and no listed field occurs in another row.

| Current owner/family and fields | Class | Reason / future authority |
|---|:---:|---|
| `Module::{globals,functions,names.link_names}` and semantic module ownership | C | Cross-function symbol and body ownership. |
| `Module::{target_triple,data_layout}` | P | Target layout policy belongs to preparation/MIR input context, not canonical transforms. |
| `Module::structured_types` including declaration/field `{name_id,fields,type_spelling}` | L | Resolve spelling to canonical types during import, then discard. |
| `Module::string_constants` and `StringConstant::{name_id,bytes}` | C | Module-owned constant objects and bytes are semantic. |
| `StringConstant::name` | X | Display spelling derived from the semantic text identity. |
| `NameTables::{texts,block_labels,slot_names}` | X | Current interning/display tables are compatibility infrastructure; future stable semantic IDs do not derive identity from spelling tables. |
| `Global::{link_name_id,type,is_extern,is_thread_local,is_constant,size_bytes,align_bytes,initializer,initializer_symbol_name_id,initializer_elements,initializer_relocation_slots}` and relocation `{byte_offset,size_bytes,target}` | C | Global definition, layout, initializer values, and symbolic relocations are module semantics. |
| `Global::{name,initializer_symbol_name}` | X | Display/raw-text duplicates when semantic link IDs exist. |
| `Global::{has_scalar_layout_authority,has_integer_array_layout_authority,integer_array_element_size_bytes,integer_array_element_count}` | L | Import-shape decisions used to construct the canonical initializer/type. |
| `Global::address_materialization_policy` | P | Target address selection belongs after canonical BIR. |
| `Function::{link_name_id,return_type,is_variadic,is_declaration,params,local_slots,blocks}` | C | Function signature/body ownership. `params` and `local_slots` are expanded below. |
| `Function::name` | X | Display duplicate of semantic symbol identity. |
| `Function::{return_size_bytes,return_align_bytes,return_abi,calling_convention,address_materialization_policy}` | P | ABI and target address policy. Calling convention is retained as preparation input metadata, not canonical instruction semantics. |
| `Function::formal_pointer_authority` | A | Interprocedural/call-graph fact, recomputable from linkage and callers. |
| `Param::{type,is_varargs,is_sret,is_byval}` | C | Signature semantics; sret/byval are source-level parameter attributes consumed by later ABI preparation. |
| `Param::name` | X | Parameter display spelling. |
| `Param::{size_bytes,align_bytes,abi}` | P | Computed ABI layout and carrier policy. |
| `LocalSlot::{slot_id,type,size_bytes,align_bytes,storage_kind,is_address_taken}` | C | Function-local storage object semantics. |
| `LocalSlot::{name,phi_observation}` | X | Spelling and old phi snapshot are compatibility observations. |
| `LocalSlot::is_byval_copy` | P | ABI copy policy is prepared output. |
| Local-array `source_objects`, `address_derivations`, `element_paths`, `semantic_geps`, `scalar_local_loads`, and global-static `semantic_geps` | C | Semantic memory/address operations must become ordinary core values/instructions, not side-table authority. |
| Local-array `selected_proof_edge_paths`, `endpoint_bridges`, `ordered_effect_source_streams`, `interval_effects`, `index_range_proofs`, `proof_facts`, `index_range_checker_inputs`, `local_address_provenances`, and global-static `gep_authorities` | A | Derived provenance, path, effect, range, and authority results recomputed from core memory/CFG. |
| `Function::atomic_operations` and `AtomicOperation::{block_label,block_label_id,inst_index}` | X | The detached positional record is legacy observation. |
| `AtomicOperation::{kind,value_type,width_bytes,result,pointer,value,expected,desired,ordering,failure_ordering,rmw_opcode,result_mode,address_space}` | C | These become fields of canonical atomic instructions rather than a side table. |
| `Block::{label_id,insts,terminator}` | C | Stable block ownership, ordered instruction sequence, and terminator-derived CFG authority. |
| `Block::label` | X | Display spelling only. |
| `Value::{kind,type,immediate,immediate_bits,f128_payload,pointer_symbol_link_name_id}` | C | Canonical constants/types/symbol references; named definitions become stable `ValueId` uses. |
| `Value::name` | X | SSA display spelling, never semantic identity. |
| `BinaryInst::{opcode,result,operand_type,lhs,rhs}` | C | Canonical computation. |
| `SelectInst::{predicate,result,compare_type,lhs,rhs,true_value,false_value}` | C | Canonical computation. |
| `CastInst::{opcode,result,operand}` | C | Canonical computation. |
| `PhiInst::{result,incomings.value,incomings.label_id}` | C | Canonical SSA join and predecessor identity. |
| `PhiIncoming::label` | X | Display/fallback predecessor spelling. |
| `CallInst::{result,result_lanes,callee_link_name_id,callee_value,args,arg_types,return_type,calling_convention,is_indirect,is_variadic,is_noreturn,intrinsic}` | C | Call/intrinsic semantics. Calling convention here is an explicit call-site semantic constraint; unlike computed ABI layout, it must survive canonical passes. |
| `CallInst::{callee,structured_return_type_name,return_type_name,sret_storage_name}` | X | Display/raw spelling duplicates. |
| `CallInst::{arg_sources,arg_abi,result_abi,va_arg_payload_abi,va_arg_hfa_lane_count,va_arg_hfa_lane_size_bytes,sret_storage_name_id}` | P | Source routing, ABI carriers, and sret placement belong to preparation. |
| `CallInst::inline_asm` including all asm text, constraint, operand, clobber, unsupported-fact, explicit-register, and `.insn` metadata | P | Target-specific instruction-selection payload. Canonical BIR may carry an opaque operation token, but these current fields are prepared/MIR output. |
| Memory instruction semantic fields: load `{result,slot_id/global_name_id,byte_offset,address}`, store `{slot_id/global_name_id,value,byte_offset,address}`; `MemoryAddress::{base_kind,base_value,slot_id,global_name_id,displacement,index,scale,address_space}` | C | Canonical memory operations and symbolic address expression. |
| Memory instruction `{slot_name,global_name}` and `MemoryAddress` spelling/provenance compatibility members | X | Display/fallback and duplicated provenance. |
| Memory instruction `align_bytes` | P | Concrete access alignment/selection policy belongs to preparation unless represented as an explicit source semantic attribute in a future schema. |
| `Inst` variant membership | C | Future opcode/instruction storage, with stable `InstId`. |
| `Terminator::{kind,value,return_lanes,condition,target_label_id,true_label_id,false_label_id}` | C | Sole CFG/return authority. |
| `Terminator::{target_label,true_label,false_label}` | X | Display/fallback labels. |
| Adapter `ValueMap`, `GlobalTypes`, `TypeDeclMap`, `ImportedFunctionSymbolIndex`, parsed typed operands, aggregate layouts, structured-layout parity tables, and temporary function/pointer/provenance maps | L | Construction-only raw LIR compatibility inputs; none may cross publication. |
| Route 1--8 record/index structs, all raw `Function*`/`Block*`/`Inst*`/`Value*`, instruction/vector/route indices, and current producer/control-flow/memory/publication/comparison/return views | A | Rebuildable analyses over a specific IR revision. Their debug serialization, if retained temporarily, is X rather than analysis authority. |
| Prealloc prepared lookup keys, call/publication plans, stack/ABI/frame/allocation facts, and target emitter selections | P | Owned by prepared BIR/MIR stage products. |

The registry therefore covers every Module, Global, structured/string, Function,
Param, LocalSlot, function side-table, Block, Value, instruction variant,
Terminator, identity-conversion, route/view, adapter-temporary, and prepared-key
family enumerated by Step 1. Split rows state a destination per individual field;
no field inherits two classes.

## `LegacyBirCompatibilityCapsule`

The capsule is an optional observational attachment to raw BIR, not a base
class and not reachable from canonical `Module`, `Function`, `Block`, `Inst`, or
analysis APIs.

```text
LegacyBirCompatibilityCapsule {
  SchemaVersion version;
  RawBirRevision observed_revision;
  LegacyFieldManifest manifest;       // sorted stable field keys
  LegacyReaderManifest readers;       // sorted stable reader keys
  LegacyDisplayTables display;
  LegacyFallbackReferences fallbacks;
  LegacyRouteSnapshots route_debug;
}

observe_legacy(raw_bir, LegacyReaderToken) -> const LegacyObservation
```

Only the raw-import publisher may populate it, once, from already-created raw
BIR. `observe_legacy` returns const data and records no semantic mutations.
There is no capsule parameter or accessor in core pass, verifier, analysis, or
mutation interfaces. Canonicalization may carry the attachment opaquely for
comparison, but must neither branch on it nor update it. A revision mismatch
makes an observation unavailable rather than silently stale.

### Initial reader allowlist

The allowlist is closed and names entry points, not directories:

| Reader key | Temporary purpose | Required exit |
|---|---|---|
| `legacy.bir_printer` | Reproduce old textual/debug spellings. | Printer derives names from core IDs or accepts an explicit external debug-name map. |
| `legacy.compat_validator` | Compare ID-backed references with raw fallback names during migration. | Core verifier rejects missing semantic IDs; compatibility import validation moves before publication. |
| `legacy.route_fixture_comparator` | Compare route 1--8 snapshots in legacy fixtures. | Tests assert recomputed analysis results through analysis APIs. |
| `legacy.prealloc_adapter` | Feed old prepared consumers until typed preparation owns all policy. | All prealloc consumers accept verified canonical BIR plus prepared outputs only. |
| `legacy.dump_renderer` | Render quarantined observations for diagnostics. | Renderer consumes explicit diagnostic snapshots and no capsule fields remain. |

No wildcard, transitive reader, target emitter, MIR component, or core pass is
allowed. Adding a reader requires an explicit migration review and is forbidden
by the monotonic gate below.

## Enforceable no-growth and deletion gates

Keep two checked-in, sorted manifests during implementation:
`legacy_bir_fields.txt` contains one stable `owner.field` key per X field/group,
and `legacy_bir_readers.txt` contains the exact reader keys above. CI computes:

```text
legacy_field_count  = non_comment_line_count(legacy_bir_fields.txt)
legacy_reader_count = non_comment_line_count(legacy_bir_readers.txt)
```

Every capsule access must be generated from or matched to one reader-manifest
key. Every capsule member must match one field-manifest key. CI rejects unknown
keys, duplicate keys, unsorted manifests, direct capsule includes outside its
implementation/allowlisted adapters, and any count greater than the preceding
checkpoint. Thus there is a no-new-writer rule (only the one raw publisher), a
no-new-field rule, and a no-new-consumer rule.

Checkpoints are monotonically decreasing pairs `(fields, readers)` recorded in
the migration ledger:

1. `Q0 capture`: freeze the initial exact counts and require all current X
   fields/readers to be manifested; subsequent pairs must be component-wise
   `<= Q0`.
2. `Q1 semantic IDs`: delete fallback symbol/block/slot/value spellings from
   non-rendering adapters and remove `legacy.compat_validator`; require at
   least one field deletion and `(fields < Q0.fields, readers < Q0.readers)`.
3. `Q2 analysis replacement`: delete route snapshots, pointer/index debug
   records, and `legacy.route_fixture_comparator`; counts strictly decrease.
4. `Q3 prepared cutover`: delete ABI/address/call-source compatibility fields
   and `legacy.prealloc_adapter`; counts strictly decrease.
5. `Q4 diagnostic cutover`: delete remaining display/fallback fields and both
   rendering readers; require `legacy_field_count == 0` and
   `legacy_reader_count == 0`.
6. `Q5 removal`: delete `LegacyBirCompatibilityCapsule`, manifests, writer, and
   access audit. CI asserts the type and `observe_legacy` symbol do not exist.

A checkpoint cannot be claimed by merely renaming or coalescing keys: CI also
records the sorted-set difference, and every removal must name the replacement
core field, analysis API, prepared output, or deleted diagnostic contract.
Rollback may restore the preceding checkpoint, never exceed its frozen counts.

## Authority summary

Core passes can observe only C and request A. The raw adapter alone consumes L.
Preparation consumes verified C plus declared target context and produces P.
X can only be observed by the shrinking allowlist and can never decide code
generation, verification, pass behavior, or analysis truth. The terminal gate
is both zero fields and zero readers, followed by deletion of the capsule itself.
