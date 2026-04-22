# X86 Codegen Rebuild Plan

## Stage-2 Audit Status

Step 1 complete: the stage-1 extraction set under
`docs/backend/x86_codegen_legacy/` is broad enough to use as redesign
evidence, but it is not yet strong enough to trust uniformly. Coverage is good
at the per-file companion level for the declared boundary
(`src/backend/mir/x86/codegen/*.cpp` and `x86_codegen.hpp`), while truthfulness
is weaker around cross-file seam pressure, trust ranking, and where the
prepared family has become a parallel lowering stack.

The older single-file summary in `docs/backend/x86_codegen_subsystem.md` is
still useful as a compressed orientation pass, but the directory extraction set
is the canonical stage-1 package and must remain the source of truth for stage
2 and stage 3.

## Boundary And Coverage Audit

What the extraction set gets right:

- `docs/backend/x86_codegen_legacy/index.md` names the intended legacy-source
  boundary and correctly frames the artifacts as evidence rather than target
  design.
- The directory contains one markdown companion for every in-scope top-level
  source named by that boundary, including the prepared files, shared helpers,
  and the mixed `x86_codegen.hpp` contract surface.
- The index already calls out the main rebuild pressure points honestly:
  `x86_codegen.hpp` contract mixing, `prepared_module_emit.cpp` dispatcher/data
  fusion, prepared predicate renderers, and
  `prepared_local_slot_render.cpp` reopening canonical ownership.

What the extraction set does not yet say clearly enough:

- The boundary excludes the `src/backend/mir/x86/codegen/peephole/` subtree.
  That may be acceptable for the current Phoenix stage, but the exclusion is a
  boundary choice rather than full-subsystem coverage and should stay explicit
  in later stage-2 and stage-3 outputs.
- The index lists families and pressure points, but it does not yet rank which
  per-file artifacts are trustworthy as-is versus which should be treated as
  weak evidence before the rebuild inherits them.
- The package is strongest at file-local summaries and weaker at cross-file
  dependency narratives such as prepared dispatch context breadth, hidden state
  flow, and which canonical seams are genuinely reusable.

## Trust Ranking Of Current Artifacts

Trustworthy as-is for stage-2 reasoning:

- `index.md` as a boundary map and first-pass ownership inventory
- `memory.cpp.md` as evidence that canonical memory/addressing already has a
  real ownership seam
- `shared_call_support.cpp.md` as evidence for low-level shared helper scope
- `route_debug.cpp.md` as a proof/debug surface rather than a lowering owner
- `x86_codegen.hpp.md` as evidence that the shared header is a rebuild pressure
  point
- `prepared_module_emit.cpp.md` and `prepared_local_slot_render.cpp.md` as
  evidence that prepared routing has grown beyond a thin client role

Weak evidence that later stages should not inherit blindly:

- `index.md` as a dependency-direction document; it correctly signals pressure
  but does not yet capture the full seam map the replacement layout needs
- `prepared_*` companions as redesign input without additional cross-file
  reconstruction; they show that prepared logic is broad, but not yet exactly
  which canonical seams are duplicated, bypassed, or still stable
- `x86_codegen_subsystem.md` if used alone; it compresses well, but it is too
  coarse to replace the stage-1 per-file package

## Extraction-Set Corrections And Expansions Required

Before stage 3 should trust this material as a drafting contract, stage 2 must
add the following missing judgments to this rebuild plan:

- a seam map that names which responsibilities are canonical, which are
  prepared-route compatibility, and which are overfit pressure to reject
- an explicit description of how prepared dispatch context pulls in naming,
  addressing, control-flow, stack-layout, and ABI concerns across file
  boundaries
- a stable-seam judgment explaining which existing APIs are worth preserving
  rather than merely restating file names
- a boundary note for the excluded peephole subtree so the replacement layout
  does not accidentally claim full x86 backend ownership when it only rebuilds
  the codegen subsystem slice

## Step-1 Outcome

The stage-1 extraction package is good enough to continue into step 2, but
only if stage 2 treats it as partially trusted evidence. The main weakness is
not missing per-file markdown companions; it is missing cross-file truth about
dependency direction, trust level, and prepared-route duplication of canonical
lowering seams.

## Current Subsystem Seam Map

### Stable canonical owners worth preserving

These are the clearest real seams in the current subsystem and should survive
the rebuild as first-class owners rather than being dissolved into another
mixed helper layer:

- call setup, call issuance, result publication, and ABI lane policy in
  `calls.cpp`
- return-value publication and final epilogue return flow in `returns.cpp`
- frame sizing, callee-save policy application, and incoming parameter home
  materialization in `prologue.cpp`
- stack-slot and indirect-memory operand formation in `memory.cpp`
- low-level ABI constants, register naming, and shared machine-policy helpers
  in `mod.cpp`
- low-level output/state helpers in `shared_call_support.cpp`
- route-proof surfaces in `route_debug.cpp`

These files are not perfectly isolated today, but they already correspond to
semantic lowering families instead of testcase-shaped renderers. The rebuild
should preserve their ownership direction and make them easier for prepared
routes to consume.

### False seams and responsibility mixing

The current subsystem also exposes several false seams that compress poorly and
hide the real dependency graph:

- `x86_codegen.hpp` is a false seam because it looks like one shared contract
  but actually mixes public entrypoints, ABI helpers, prepared dispatch
  contexts, low-level utility declarations, and renderer-specific surfaces.
- `prepared_module_emit.cpp` is a false seam because it fuses dispatcher
  decisions with concrete `.rodata`/`.data`/`.bss` emission, symbol naming,
  and prepared renderer selection.
- `prepared_local_slot_render.cpp` is a false seam because it looks like a
  local-slot renderer but in practice reopens frame-layout, value-home,
  byval/HFA, address formation, and call-lane behavior that already have
  canonical owners elsewhere.
- `prepared_param_zero_render.cpp` and `prepared_countdown_render.cpp` are
  false seams when treated as standalone branch owners; their narrow helper
  value is real, but predicate and branch shaping belong to shared comparison
  and control-flow lowering.

### Hidden dependency direction the replacement must make explicit

The extraction set shows that prepared routing is currently fed by a broad
context object whose consumers need all of the following at once:

- prepared module and function metadata
- stack layout and frame-slot decisions
- prepared addressing and value-home lookup
- prepared control-flow and block lookup
- symbol naming and global/string data queries
- ABI register naming and helper-emission callbacks

That breadth is the real seam problem. The prepared family is not just a
consumer of canonical lowering; it often carries enough hidden knowledge to
bypass canonical owners and render final assembly directly. The replacement
must turn those implicit dependencies into explicit, narrow query surfaces.

## Failure Pressure From Idea 75

Idea 75 is the forcing function for this rebuild. Its durable note says the
truthful publishers already exist upstream: stack-layout publication, regalloc
publication, and prepared move-bundle facts are already separately correct for
the motivating `00204.c` runtime leaf. The remaining defect appears downstream
when the x86 consumer lowers those truthful prepared facts into overlapping
stack writes before the call.

That matters for stage 2 because it changes the diagnosis:

- the next defect is not another missing publication fact
- the next defect is not parse/semantic/frontend drift
- the next defect is not a broad “prepared route unsupported” bucket
- the next defect is the consumer seam where truthful prepared homes and
  addressing are turned into final x86 operands and call-lane copies

The current seam map explains that pressure: `prepared_local_slot_render.cpp`
and adjacent prepared renderers are carrying enough lowering ownership to
mis-handle call-lane ordering and address formation without going through one
canonical call/memory/frame seam. The rebuild must therefore make prepared
routes consume canonical ownership instead of growing a second lowering stack.

## Replacement Layout

### Design rules

The replacement layout must enforce these rules:

- no replacement header may mix public entrypoints, ABI helpers, prepared
  dispatch contexts, and renderer-specific declarations the way
  `x86_codegen.hpp` does today
- prepared routes may select bounded fast paths, but they must consume shared
  call, return, frame, memory, and comparison seams instead of rendering those
  policies from scratch
- module/data emission must be separate from route selection
- route-debug surfaces must read routing facts only and must not own lowering
- optional fast paths must stay thin adapters with explicit fallback rules, not
  open-ended matcher growth buckets
- compatibility surfaces must be named explicitly so stage 3 does not smuggle
  legacy coupling back in under generic helper names

### Mandatory replacement draft manifest

Stage 3 must produce the following draft set under
`docs/backend/x86_codegen_rebuild/`.

Directory/index artifacts:

- `index.md`
- `layout.md`

Mandatory replacement headers:

- `api/x86_codegen_api.hpp.md`
- `core/x86_codegen_types.hpp.md`
- `core/x86_codegen_output.hpp.md`
- `abi/x86_target_abi.hpp.md`
- `module/module_emit.hpp.md`
- `module/module_data_emit.hpp.md`
- `lowering/frame_lowering.hpp.md`
- `lowering/call_lowering.hpp.md`
- `lowering/return_lowering.hpp.md`
- `lowering/memory_lowering.hpp.md`
- `lowering/comparison_lowering.hpp.md`
- `lowering/scalar_lowering.hpp.md`
- `lowering/float_lowering.hpp.md`
- `lowering/atomics_intrinsics_lowering.hpp.md`
- `prepared/prepared_query_context.hpp.md`
- `prepared/prepared_fast_path_dispatch.hpp.md`
- `prepared/prepared_fast_path_operands.hpp.md`
- `debug/prepared_route_debug.hpp.md`

Mandatory replacement implementation drafts:

- `api/x86_codegen_api.cpp.md`
- `core/x86_codegen_output.cpp.md`
- `abi/x86_target_abi.cpp.md`
- `module/module_emit.cpp.md`
- `module/module_data_emit.cpp.md`
- `lowering/frame_lowering.cpp.md`
- `lowering/call_lowering.cpp.md`
- `lowering/return_lowering.cpp.md`
- `lowering/memory_lowering.cpp.md`
- `lowering/comparison_lowering.cpp.md`
- `lowering/scalar_lowering.cpp.md`
- `lowering/float_lowering.cpp.md`
- `lowering/atomics_intrinsics_lowering.cpp.md`
- `prepared/prepared_fast_path_dispatch.cpp.md`
- `prepared/prepared_fast_path_operands.cpp.md`
- `debug/prepared_route_debug.cpp.md`

### Ownership of the replacement files

The replacement manifest is intentionally organized by dependency direction:

- `api/` owns public entrypoints only; it should not own mixed helper state
- `core/` owns shared data structures and output helpers used by the lowering
  families
- `abi/` owns target profile and ABI facts, register naming, and stable policy
- `module/` owns top-level module orchestration and data-section emission, with
  route choice separated from data rendering
- `lowering/` owns canonical semantic families such as frame, call, return,
  memory, comparison, scalar, float, and atomics/intrinsics lowering
- `prepared/` owns prepared-route query adaptation and thin fast-path
  selection, but not independent ABI or frame semantics
- `debug/` owns route summaries and traces as proof/debug consumers

### Old-to-new responsibility mapping

The critical responsibility moves are:

- split `x86_codegen.hpp` into `api/`, `core/`, `abi/`, `prepared/`, and
  `debug/` headers so declarations follow ownership direction
- split `prepared_module_emit.cpp` into `module/module_emit.cpp` for top-level
  orchestration and `module/module_data_emit.cpp` for symbol/data emission
- replace `prepared_local_slot_render.cpp` with thin prepared adapters that
  query canonical `frame_lowering`, `call_lowering`, and `memory_lowering`
  seams instead of owning stack/address/call policy themselves
- replace standalone branch-shape ownership in `prepared_param_zero_render.cpp`
  and `prepared_countdown_render.cpp` with bounded fast-path dispatch that
  consumes shared `comparison_lowering` services
- preserve `route_debug.cpp` as a proof surface, but move it behind an
  explicit debug header and remove lowering knowledge from it

## Stage-3 Readiness

Stage 3 is ready to draft only if it follows this plan as a contract rather
than a suggestion. The draft stage must not recreate one giant shared header,
must not keep module-data emission fused with prepared dispatch, and must not
let prepared fast paths own their own private copies of frame, address, call,
or comparison policy.
