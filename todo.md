Status: Active
Source Idea Path: ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Map `store_source_publications` Structural Exit Preconditions

# Current Packet

## Just Finished

Step 5 - Map `store_source_publications` Structural Exit Preconditions
completed as an analysis-only map for
`PreparedBirModule::store_source_publications`.

`store_source_publications` structural exit map:

- Direct aggregate field production and direct readers:
  `publication_plans.cpp::populate_store_source_publication_plans` clears and
  appends `PreparedStoreSourcePublicationRecord` rows to
  `prepared.store_source_publications.records`. The only direct field reader
  found outside construction is
  `prepared_printer/select_chains.cpp::append_store_source_publications`,
  which prints `store_source` rows. AST type-reference checks found
  `PreparedStoreSourcePublicationPlans` in `publication_plans.cpp` only at
  the append helper and `PreparedStoreSourcePublicationRecord` in the prepared
  printer row helper. No non-printer production reader of the stored aggregate
  records was found; this must be treated as blocked/needs confirmation before
  any deletion, privatization, or field-retirement packet is named.
- Source/publication semantic record candidates:
  `plan_prepared_store_source_publication` records source value identity,
  source value name/id from source homes or memory access stored-value names,
  source producer kind, producer block/instruction identity, producer
  instruction pointers, recovered narrow-store source value/instruction,
  byval load-local source, and direct-global select-chain dependency. These
  overlap route/source facts and can be future semantic candidates only one
  row at a time: source value identity, producer identity, recovered narrow
  store source, byval pointer-source classification, or direct-global
  select-chain dependency. The record itself also carries destination,
  storage, and publication policy, so the whole `PreparedStoreSourcePublicationPlan`
  is not a semantic exit.
- Source-memory status compatibility:
  `PreparedStoreSourcePublicationStatus` is `Available`,
  `MissingSourceValue`, or `MissingDestinationAccess`, and
  `prepared_store_source_publication_available` gates later readers. Missing
  source values and missing destination accesses intentionally fail closed and
  still produce status rows for compatibility/printer evidence. These statuses
  are not deletion candidates; any route-backed source row must preserve
  missing-source, missing-access, invalid value name/id, absent home, absent
  addressing, and mismatched access behavior by returning unavailable,
  `std::nullopt`, or no emitted target record.
- Publication status and intent compatibility:
  `PreparedStoreSourcePublicationIntent` distinguishes none, store-local
  publication, store-global publication, and pointer-store writeback.
  Plan flags `pending_publication`, `stack_homes_only`,
  `pointer_store_writeback`, and `duplicate_publication` are consumed by
  AArch64 store-local, store-global, pending publication, and pointer-writeback
  paths. These are publication mechanics and compatibility status, not plain
  source semantics. Fail-closed proof for any helper migration must include
  wrong intent, duplicate publication, non-pending vs pending, stack-homes-only
  policy, pointer-writeback policy, and unsupported intent rows.
- Addressing and storage output consumers:
  AArch64 `memory.cpp` recomputes store-source plans through
  `plan_store_local_source_publication`,
  `plan_stack_homed_pointer_store_writeback`,
  `plan_pointer_base_plus_offset_store_local_publication`,
  `plan_fixed_formal_store_local_publication`, and
  `plan_prepared_store_global_publication`. The lowering then consumes
  destination base kind, destination pointer/frame-slot ids, destination byte
  offset, stack offsets, source home kind, source storage encoding, pointer
  base homes, stack object/slot details, volatile/access facts, and target
  scratch/register availability to decide emitted instructions. These are
  target/addressing/storage output surfaces and remain blocked unless a later
  packet names one helper row and preserves all target fallback behavior.
- Producer and recovered-source target-policy consumers:
  AArch64 `memory.cpp` uses source producer completeness helpers for cast,
  select materialization, scalar FP binary, global-symbol load-local, direct
  global select chains, recovered narrow-store values, byval load-local
  sources, fixed formal stores, future stack-value publication coverage, and
  pending store-global stack publications. `dispatch_value_materialization.cpp`
  also calls `find_prepared_recovered_narrow_store_source_for_wide_local_load`
  before rematerializing load-local values. These readers are production
  consumers of planner/helper facts, not direct readers of
  `store_source_publications.records`. They can be implementation candidates
  only if scoped to a single helper fact and proven to fail closed for absent
  prepared context, absent control-flow block, invalid block label, missing
  producer, wrong producer kind, producer/block mismatch, missing access,
  source/destination mismatch, unsupported type/width, and target scratch or
  register policy failure.
- Printer rows and public aggregate compatibility:
  `append_store_source_publication_row` prints function, block, instruction,
  source, status, intent, producer kind, producer block/instruction, producer
  pointer booleans, and direct-global select-chain fields from the stored
  aggregate record. These rows are byte-stable debug/oracle output, not
  semantic exits. The public aggregate field remains a compatibility surface
  until a non-printer production reader is confirmed or an explicit printer
  compatibility replacement is scoped.
- Target-policy fallback:
  Store-source publication planning is reused as an oracle in AArch64 lowering
  to choose whether to emit a specialized publication or fall back to the
  ordinary lowered memory instruction. This includes casts, selects,
  direct-global chains, global-symbol loads, pointer base-plus-offset stores,
  fixed-formal stores, pending store-global publication, future store-local
  stack publication coverage, ABI scratch availability, register class checks,
  mnemonic availability, and memory operand support checks. These target
  fallback decisions are retained target policy and are blocked from structural
  exit.

Concrete future packets:

- Helper-row packet candidate:
  `find_prepared_recovered_narrow_store_source_for_wide_local_load` can be
  scoped as a recovered-source identity packet. Reader: prealloc population,
  AArch64 store-local publication planning, and AArch64 dispatch value
  materialization. Semantic fact: a same-block wide load-local can recover the
  immediately prior narrower store-local value that targets the loaded byte.
  Compatibility surface: addressing, stack-layout slot/object checks,
  load-result type, lane offset parsing, store width, and instruction ordering
  still own availability. Fail-closed proof: null block, null addressing,
  invalid block label, load/access mismatch, missing frame slot, non-integer
  load, no prior store, prior store/access mismatch, wrong byte lane,
  non-8-bit store, store width not narrower than load, and multiple earlier
  stores must not invent a recovered value.
- Helper-row packet candidate:
  `prepared_store_source_load_local_is_byval_formal_pointer_source` can be
  scoped as a byval pointer-source classification packet. Reader: prealloc
  population and AArch64 store-local source publication planning. Semantic
  fact: the load-local source producer reads through a pointer-value memory
  access whose pointer value name is a byval formal. Compatibility surface:
  prepared addressing and formal-name lookup remain authoritative. Fail-closed
  proof: null addressing, null producer, wrong producer kind, missing
  load-local pointer, invalid producer block label, missing memory access,
  non-pointer base kind, missing pointer value name, base-plus-offset false,
  non-byval formal, and prepared/BIR name mismatch all return false.
- Helper-row packet candidate:
  `find_prepared_store_source_direct_global_select_chain_dependency` can be
  scoped as a direct-global select-chain dependency packet. Reader: prealloc
  population and AArch64 store-local publication planning. Semantic fact: the
  store source depends on a same-block select/direct-global load producer
  before the store. Compatibility surface: source producer lookups, block
  identity, before-instruction cutoff, root-is-select flag, and root
  instruction index remain row policy. Fail-closed proof: null names/query,
  null source producers, invalid block label, null block, non-named value,
  unknown value name, producer after the store, producer in another block,
  missing direct-global load, mismatched select root, and missing root
  instruction index all keep dependency fields false/empty.
- Helper-row packet candidate:
  `plan_prepared_store_source_publication` may be split only for the narrow
  source-value/source-producer metadata copy into a plan row. Reader: plan
  construction for prealloc records and AArch64 recomputed plans. Semantic
  fact: the row carries the same BIR source value and route/prepared producer
  identity for the named store source. Compatibility surface: status, intent,
  destination access, source home, storage encoding, recovered/direct-global
  flags, pointer-base homes, and duplicate/pending policy remain in the
  prepared planner. Fail-closed proof: missing source, missing destination,
  invalid source value name, absent source home, unknown producer, wrong
  producer kind, producer block mismatch, source value mismatch, and
  prepared/BIR name mismatch preserve current status and empty producer fields.

Blocked exits:

- Whole-field `PreparedBirModule::store_source_publications` deletion,
  privatization, or aggregate retirement is blocked because no non-printer
  production reader of the stored records was found; this needs confirmation
  before deletion-ready claims.
- Prepared-printer `store_source` rows and expected text are output
  compatibility surfaces, including status/intent strings and producer boolean
  columns.
- `PreparedStoreSourcePublicationStatus`, intent, pending/duplicate,
  stack-homes-only, pointer-writeback, and availability gates are retained
  publication compatibility.
- Destination access, frame-slot/object, stack offset/size/align,
  pointer-base, storage encoding, volatile/access facts, and memory operand
  support are addressing/storage output surfaces.
- AArch64 store-local/store-global lowering, fixed-formal stores,
  pointer-store writeback, pending stack publication, future-publication
  coverage, scratch/register selection, mnemonic selection, and emitted
  assembler text are target-policy fallback surfaces.
- Broad publication planner rewrites, prepared aggregate construction changes,
  and expectation rewrites are out of Step 5 scope.

## Suggested Next

Proceed to Step 6 by normalizing the blocker map and proof needs across
`module`, `names`, `control_flow`, and `store_source_publications` into a
compact supervisor/reviewer handoff.

## Watchouts

Do not treat `store_source_publications.records` as deletion-ready: no
non-printer production reader was found, so the direct field is
blocked/needs-confirmation rather than proven removable. Future packets must
name one helper row and keep status, intent, addressing, storage, printer, and
target fallback behavior intact.

## Proof

Analysis-only packet; no build or test proof required. Ran
`git diff --check -- todo.md`.
