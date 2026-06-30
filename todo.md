Status: Active
Source Idea Path: ideas/open/442_pointer_value_memory_provenance_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Producer Publication Or Policy Coverage

# Current Packet

## Just Finished

Completed Step 2: selected the first bounded producer packet for idea 442
without changing implementation files or tests.

Selected packet: `Publish Same-Module Computed-Address Formal Provenance`.

Route: concrete provenance publication, not opaque compatibility.

Scope:

- Target the three `930930-1` pointer-value memory loads through
  `%lv.param.reg1`: `f:block_4:inst1`, `f:logic.rhs.11:inst1`, and
  `f:block_5:inst1`.
- These derive from formal `%p.reg1`, whose caller-side arg3 is already
  recorded as `source_base=@mem source_delta=792`.
- Publish only enough producer/prepared authority for resulting
  pointer-value memory records to pass
  `prepare::prepared_pointer_value_memory_has_proven_authority`.

Contract:

- direct same-module call with available callee definition;
- non-sret, non-byval pointer formal;
- coherent computed-address argument source for the selected formal;
- concrete source object with known complete extent and known source delta;
- provenance preserved through the callee formal local store/load chain;
- final prepared pointer-value memory record has concrete non-bare base
  identity, non-unknown layout authority, complete extent, matching requested
  range, and `range_verdict=ProvenInBounds`.

Rejected adjacent shapes:

- opaque compatibility for arbitrary runtime pointers;
- weakening `prepare::prepared_pointer_value_memory_has_proven_authority`;
- RV64-side provenance/range inference;
- indirect/external calls, missing callee definitions, variadic ambiguity,
  missing call-argument relationships, frame-slot value args, symbol-only args
  without computed-address provenance, dynamic or incoherent bases, and
  multiple inconsistent callsites;
- store-source/global-memory publication, direct-global return/select-chain,
  terminator/select publication, expectation rewrites, unsupported-marker
  edits, allowlists, and pass/fail accounting.

Pointer-delta decision: split it. The `f:block_5:inst5` store through
`%t27 = %t26 - 8` remains a concrete-provenance candidate, but it requires
constant pointer add/sub propagation from `%p.mr_TR`/`%lv.param.mr_TR` to
`%t27`. That should be the follow-up packet after call/formal/local-load
publication is proven.

Artifact:

- `build/agent_state/442_step2_provenance_packet_selection/selection.md`

## Suggested Next

Execute Step 3: Implement Producer Publication Or Policy Coverage.

Recommended Step 3 packet: implement `Publish Same-Module Computed-Address
Formal Provenance`.

Owned files:

- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp` only if existing
  `PointerAddress` cannot express the needed facts
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/442_step3_concrete_call_arg_provenance/*`

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Expected Step 3 evidence: focused producer/prepared tests prove same-module
computed-address pointer arguments seed callee pointer provenance and the three
`%reg1` pointer-value memory records classify as proven authority. The
`%mr_TR - 8` store may remain fail-closed until the pointer-delta packet.

## Watchouts

- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Do not infer pointer-value memory provenance, layout, object extent, or range
  in RV64 from raw pointer values, integer casts, filenames, function names,
  value names, object labels, or dump shape.
- Treat `layout_authority=unknown` and `range_verdict=UnknownCompatible` as
  fail-closed unless a named and tested opaque-compatibility policy is created.
- Keep store-source/global-memory publication, direct-global
  return/select-chain, and terminator/select publication out of this plan.
- Do not edit RV64 lowering for this producer route.
- Do not key the route to `930930-1`, `reg1`, `mr_TR`, one block label, or one
  dump layout; use same-module call argument and prepared provenance facts as
  authority.
- Do not include pointer-delta propagation in the first packet unless the
  implementation proves it is inseparable from call/formal/local-load
  provenance.

## Proof

Step 2 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
