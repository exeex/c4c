Status: Active
Source Idea Path: ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Stack Layout Reconstruction Routes

# Current Packet

## Just Finished

Step 1 inventory completed for stack-layout reconstruction routes that infer
slot families, aggregate slice exposure, address publication, pointer roots, or
coalescing facts from names and BIR instruction scans.

Name-based family inference routes:
- `analysis.cpp` `slot_slice_family`: parses a trailing numeric suffix after
  the last dot in a slot name and treats the prefix as an aggregate/slice
  family. Used by `apply_aggregate_address_publication_hints` to map a
  published pointer value named like the family root to all slot objects whose
  names look like `family.N`. Current behavior: target-neutral family/publication
  relation is reconstructed from spelling. Likely owner: BIR/lowering should own
  slice-family publication if this is semantic; prealloc may own only the
  home-slot hint. Contract gap: no explicit BIR/prepared family carrier says
  that scalarized slots belong to a published aggregate pointer.
- `slot_assignment.cpp` `build_slice_family_layout_map`,
  `belongs_to_fixed_slice_family`, and `find_object_slice_layout`: parse
  prepared slot names with `parse_prepared_slot_slice_name`, group slots by
  prefix, infer dense/non-dense families, preserve slice offsets when needed,
  and force whole fixed-location families into fixed placement when any member
  needs a fixed home. Current behavior: physical placement analysis uses
  name-derived family facts. Likely owner: prealloc stack layout for frame-slot
  ordering/offsets, but the family membership input is reconstructed. Contract
  gap: decide whether the spelling-derived family is documented placement-only
  compatibility or should consume explicit slice-family metadata.
- `coordinator.cpp` private `parse_slot_slice_name`,
  `build_slot_slice_coverage`, and `find_direct_frame_slot`: reconstruct slice
  coverage from prepared slot names so an access to `family.0 + byte_offset`
  can publish a `PreparedMemoryAccess` for the covering frame slot such as
  `family.1` or `family.2`. Current behavior: prepared address publication for
  local slots depends on name-derived slice coverage. Likely owner: prealloc
  prepared-address publication, but the family/coverage fact is target-neutral.
  Contract gap: this is more than physical placement; it affects published
  memory-address authority consumed by later stages.
- `frame.hpp` `find_prepared_stack_frame_offset_by_name`: public helper repeats
  slice-name parsing to resolve frame offsets for a requested name or covering
  slice. Current behavior: display/name helper reconstructs slot-family
  coverage after stack layout. Likely owner: prepared frame lookup helper.
  Contract gap: if retained, it needs the same status as coordinator slice
  coverage to avoid a second semantic spelling authority.

BIR instruction scans for aggregate slice exposure and address publication:
- `analysis.cpp` `apply_aggregate_address_publication_hints`: scans all calls
  except runtime intrinsic placeholders and skips byval-copy args; it records
  pointer-typed named call args and pointer values stored to locals as published
  pointer values. Then it name-matches those values to slice-family prefixes and
  marks every matching slice object `requires_home_slot` and
  `permanent_home_slot`. Current behavior: BIR instruction scan plus slot-name
  family reconstruction publishes aggregate family exposure. Classification:
  target-neutral family/publication fact reconstructed in prealloc. Likely
  owner: BIR/lowering or prepared stack-object metadata for aggregate
  publication; prealloc owns the resulting home-slot decision.
- `coordinator.cpp` `append_frame_slot_address_materialization`: treats a
  pointer result name as a slot name and materializes a frame-slot address when
  a frame slot with the same prepared slot name exists. Current behavior:
  prepared address-materialization publication infers local-slot address identity
  from result-name/slot-name equality. Classification: address-publication
  inference, not pure physical placement. Likely owner: BIR value metadata or
  explicit prepared frame-address materialization fact. Proof gap: existing
  tests cover string/global/label materialization more directly than local
  frame-address name equality.
- `coordinator.cpp` `build_pointer_indirect_address` and pointer-indirect access
  builders: publish pointer-value memory accesses from explicit
  `MemoryAddress::PointerValue` facts. Current behavior: fail-closed structured
  input for pointer-indirect prepared addresses; no slot-family reconstruction.
  Classification: fail-closed input / prepared address fact consumer. Likely
  owner: BIR memory-address producer plus prealloc prepared addressing.

Pointer-root inference used by stack-layout hints:
- `alloca_coalescing.cpp` `collect_slot_use_summary`: scans BIR instructions,
  builds a pointer alias map, and marks root local slots as used or
  address-exposed through `LocalSlot`, `PointerValue`, calls, returns,
  conditional branches, stores, casts, phi/select merges, and pointer binary
  operations. It uses raw `Value::name` equality against local-slot names and
  propagated aliases to reconstruct pointer roots. Current behavior: conservative
  prealloc alloca/home-slot analysis; roots can force `address_exposed` and
  `requires_home_slot`. Classification: mostly physical placement analysis, but
  pointer-root publication/exposure is target-neutral and reconstructed from
  instruction shape. Likely owner: prealloc can own conservative placement
  hints; BIR/prepared should own any durable pointer-root fact needed outside
  home-slot decisions.
- `alloca_coalescing.cpp` `record_memory_address_use`: explicit
  `MemoryAddress::LocalSlot` marks a slot addressed directly; explicit
  `MemoryAddress::PointerValue` follows pointer aliases and marks roots escaped.
  Current behavior: structured memory-address input for some cases, but root
  propagation remains scan-derived. Classification: mixed structured input plus
  placement analysis. Proof gap: tests cover pointer-addressed local/global
  roots, but not every alias transform as a contract family.
- `alloca_coalescing.cpp` `record_call_pointer_uses`: `sret_storage_name` marks
  a direct sret storage slot and call args/callee values escape pointer roots.
  Current behavior: BIR call metadata plus scan-derived roots controls
  `source_kind = call_result_sret`, permanent homes, and exposure. Classification:
  target-neutral call storage fact for sret, placement analysis for the final
  home-slot result. Likely owner: BIR call metadata for sret, prealloc stack
  layout for slot placement.
- `regalloc_helpers.cpp` `apply_regalloc_hints` and `inline_asm.cpp`
  `summarize_inline_asm`: scan local-slot metadata and inline asm calls; if
  inline asm has side effects and a slot is address-exposed, force a permanent
  home. Current behavior: conservative placement-only analysis. Likely owner:
  prealloc/regalloc bridge. Contract gap is low unless address-exposed itself
  remains scan-derived from earlier routes.

Alloca and copy coalescing paths depending on reconstructed facts:
- `alloca_coalescing.cpp` final object mutation: uses scan summary to mark dead
  local slots, addressed local slots, direct accesses, sret storage,
  rooted-pointer-bookkeeping-only cases, and multi-block use as
  `address_exposed`, `requires_home_slot`, or `source_kind = call_result_sret`.
  Current behavior: conservative physical placement/coalescing hint. Likely
  owner: prealloc stack-layout analysis, with source facts from BIR scans.
  Contract gap: decide which outputs are only home-slot decisions versus durable
  publication facts.
- `copy_coalescing.cpp` `summarize_slot_accesses` and
  `is_copy_coalescing_candidate`: scans raw `LoadLocalInst::slot_name` /
  `StoreLocalInst::slot_name`, requires exactly one store and one load in the
  same block on a lowering-scratch, non-address-taken, non-byval slot, then
  marks `source_kind = copy_coalescing_candidate`, clears
  `requires_home_slot`, and clears `address_exposed`. Current behavior:
  placement-only copy-elision hint reconstructed from instruction shape.
  Likely owner: prealloc stack layout. Contract gap: proof should ensure it
  stays local and does not become semantic copy authority.
- `slot_assignment.cpp` `uses_copy_coalesced_slot`: drops
  `copy_coalescing_candidate` objects from frame-slot assignment. Current
  behavior: physical placement decision based on previous scan hint. Likely
  owner: prealloc stack layout.

Existing proof surfaces:
- `backend_prepare_stack_layout_test.cpp` covers copy coalescing, dead slots,
  addressed slots, fixed-location/gap-fill placement, permanent homes, byval and
  sret params, sret storage, scalarized call-escaped families, pointer-addressed
  local/global roots, store/phi/select/cast/return/cond-branch/pointer-binary
  escape cases, and sliced i16 local-address coverage.
- `backend_aarch64_instruction_dispatch_test.cpp` has downstream assembly/MIR
  checks for local aggregate address publication and fixed-offset local copies,
  but those are less direct than prepared stack-layout assertions.

Candidate contract gaps for Step 2:
- Decide whether suffix-derived slot families (`family.N`) are retained as
  documented compatibility/placement analysis or replaced by explicit
  BIR/prepared slice-family facts.
- Decide whether aggregate address publication from call/store scans should
  consume explicit publication metadata instead of matching pointer value names
  to family prefixes.
- Decide whether coordinator/frame helper slice coverage is allowed to publish
  prepared memory facts from names or must consume structured slice coverage.
- Decide which alloca pointer-root scans are placement-only and which expose
  missing target-neutral root/publication facts.
- Keep copy coalescing classified as prealloc placement-only unless a future
  step needs broader liveness-backed copy authority.

## Suggested Next

Execute `plan.md` Step 2: decide the slice-family/publication fact contract,
including retained placement-only paths, explicit BIR/prepared fact targets, and
focused proof targets for address publication plus coalescing/slice-family
behavior.

## Watchouts

- Do not move frame offsets, stack object ordering, or home-slot placement into
  BIR.
- Do not treat every BIR scan as duplication without naming the reconstructed
  fact.
- Separate prealloc placement-only hints from target-neutral slice-family or
  publication facts.
- The highest-risk routes are the ones that publish prepared memory/address
  facts (`find_direct_frame_slot`, `append_frame_slot_address_materialization`)
  rather than merely deciding home-slot placement.
- Existing tests are broad, but Step 2 should pick focused proof cases rather
  than relying only on the large stack-layout omnibus.

## Proof

Passed. Ran:
`git diff --quiet -- src/backend/bir src/backend/prealloc tests && printf 'analysis-only proof: no implementation or test diff for stack-layout reconstruction inventory\n' > test_after.log`

Proof log: `test_after.log`.
