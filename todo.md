Status: Active
Source Idea Path: ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide The Slice-Family Fact Contract

# Current Packet

## Just Finished

Step 2 contract decision completed for stack-layout slice-family and
publication facts.

Chosen contract:
- Keep physical stack layout in prealloc: object collection, frame-slot
  assignment, offsets, frame size/alignment, fixed/reorderable placement, gap
  filling, and copy-elision home-slot decisions remain prealloc authority.
- Split target-neutral family/publication facts from physical placement. A
  route may scan BIR or names only when its output is a named placement-only
  hint; any route that publishes prepared memory/address facts must consume an
  explicit prepared/BIR fact or fail closed.
- Reject helper-only rename routes. Step 3 must either narrow a reconstruction
  boundary, add/consume a structured fact, or clearly label a retained
  placement-only compatibility class. Pure renaming while preserving semantic
  name reconstruction is not progress.

Explicit fact targets for Step 3:
- Add a structured prepared slice-family relation for stack objects, carrying
  family/root identity and slice offset/coverage for scalarized aggregate slots.
  This may be produced from current BIR/local-slot metadata initially, but once
  published it becomes the authority consumed by stack-layout assignment,
  prepared memory access publication, and frame-offset lookup.
- Add a structured prepared aggregate address-publication fact or marker that
  records when an aggregate family/root address is exposed by call/store
  behavior. `apply_aggregate_address_publication_hints` may remain the first
  producer in this route, but matching pointer values to `family.N` spelling
  should not remain the semantic authority after the fact is published.
- Add a structured frame-slot address-materialization authority for local
  frame-address values. `append_frame_slot_address_materialization` should not
  infer address identity solely from result-name/slot-name equality unless it is
  explicitly retained as a narrow compatibility path.

Retained placement-only paths:
- `slot_assignment.cpp` may continue using slice-family metadata to decide
  fixed-location family grouping, offset preservation, and frame-slot ordering.
  These are physical placement decisions and must not move into BIR.
- `alloca_coalescing.cpp` pointer-root scans may remain as named conservative
  alloca/home-slot placement analysis when they only set `address_exposed`,
  `requires_home_slot`, `permanent_home_slot`, or `source_kind`. They should not
  become durable pointer-root/publication authority outside stack-layout
  placement.
- `copy_coalescing.cpp` may remain a named placement-only copy-elision hint:
  same-block one-store/one-load lowering scratch, non-address-taken, non-byval.
  It must not claim semantic copy authority or cross-block/liveness authority.
- `regalloc_helpers.cpp` and `inline_asm.cpp` remain conservative placement-only
  home-slot reinforcement.

Compatibility paths retained only if needed:
- Suffix-derived `family.N` parsing may remain temporarily as
  `LegacySlotNameSliceFamilyCompatibility` only as a producer/bootstrap for the
  new structured prepared slice-family relation. It should be isolated and
  documented as compatibility, not used directly by consumers that publish
  prepared memory/address facts.
- Result-name/slot-name equality for frame-address materialization may remain
  only as `LegacyFrameAddressNameCompatibility` if Step 3 cannot express all
  current fixtures with explicit BIR/prepared metadata in one narrow slice.
  If retained, tests must prove it is narrow and not the primary authority when
  explicit metadata exists.

Implementation targets:
- Introduce a prepared slice-family metadata surface near `PreparedStackObject`
  or adjacent stack-layout facts, then route `slot_assignment.cpp`,
  coordinator slice coverage, and `frame.hpp` lookup through it.
- Replace direct `parse_slot_slice_name` consumer authority in coordinator/frame
  helpers with structured slice-family lookup. Keep frame offsets/slot IDs
  produced by prealloc.
- Publish aggregate address exposure as an explicit prepared fact before it
  mutates `requires_home_slot`/`permanent_home_slot`.
- Keep alloca and copy coalescing implementation narrow: classify helper names
  and comments only where they clarify placement-only status; do not rebuild
  coalescing broadly.

Focused proof targets:
- Address-publication proof: a sliced/local aggregate access such as
  `family.0 + offset` resolves through structured slice-family coverage to the
  covering frame slot, and a malformed or unstructured slice spelling fails
  closed instead of silently publishing a prepared access.
- Frame-address materialization proof: local frame-address materialization uses
  explicit prepared/BIR authority, or if compatibility remains, the test names
  and bounds that compatibility.
- Coalescing/slice-family proof: copy coalescing stays placement-only and local
  to the one-store/one-load lowering-scratch case; aggregate slice-family
  fixed-location/offset preservation remains driven by structured family
  metadata rather than direct name parsing in consumers.
- Pointer-root placement proof: at least one pointer-root escape path remains a
  conservative home-slot placement hint without becoming prepared publication
  authority.

## Suggested Next

Execute `plan.md` Step 3: implement the structured slice-family/publication
boundary narrowly, keeping frame placement in prealloc and avoiding broad
alloca/copy coalescing rewrites.

## Watchouts

- Do not move frame offsets, stack object ordering, or home-slot placement into
  BIR.
- The highest-risk routes are the ones that publish prepared memory/address
  facts (`find_direct_frame_slot`, `append_frame_slot_address_materialization`)
  rather than merely deciding home-slot placement.
- Step 3 should avoid a helper-only rename. The diff must create or consume a
  structured fact, or explicitly isolate a named compatibility path.
- Do not broaden into unrelated prepared memory, pointer-carrier, alloca, or
  copy-coalescing rewrites.

## Proof

Passed. Ran:
`git diff --quiet -- src/backend/bir src/backend/prealloc tests && printf 'analysis-only proof: no implementation or test diff for stack-layout slice-family contract decision\n' > test_after.log`

Proof log: `test_after.log`.
