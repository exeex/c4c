Status: Active
Source Idea Path: ideas/open/442_pointer_value_memory_provenance_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-probed `930930-1` pointer-value memory residuals and
recorded close-readiness disposition for idea 442.

Fresh prepared evidence:

- The call to `f` still publishes computed-address sources for all pointer
  arguments, including `index=3 source_base=@mem source_delta=792` and
  `index=4 source_base=@mem source_delta=792`.
- `930930-1::f` remains external-linkage, so observed same-module direct calls
  are not complete authority for the formal pointer values.
- The three `%lv.param.reg1` pointer-value loads remain fail-closed:
  `f:block_4:inst1`, `f:logic.rhs.11:inst1`, and `f:block_5:inst1` all report
  `layout_authority=unknown range_verdict=unknown_compatible`.
- The pointer-delta row `f:block_5:inst5`, through `%t27 = %t26 - 8`, also
  remains `layout_authority=unknown range_verdict=unknown_compatible`.

Fresh RV64 object evidence:

- `--codegen obj` still stops before success with
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- This does not change the producer-owner verdict: the prepared pointer-value
  memory rows are still non-authoritative and must remain fail-closed for RV64
  target consumers.

Disposition:

- The Step 3 internal-only same-module computed-address formal provenance rule
  is sound and covered, but it does not solve the external-linkage
  representative.
- Idea 442 is not close-ready for `930930-1` external-linkage formal
  provenance publication.
- The next lifecycle action should be a split/follow-up producer initiative for
  reliable closed-world, internal/private, or no-external-caller authority
  before any external-linkage formal pointer provenance is published.
- Pointer-delta propagation remains a later packet only after base formal
  pointer provenance is soundly authorized; `%mr_TR - 8` should not be
  implemented first for `930930-1`.

Artifacts:

- `build/agent_state/442_step4_residual_disposition/classification.md`
- `build/agent_state/442_step4_residual_disposition/evidence_snippets.txt`
- `build/agent_state/442_step4_residual_disposition/930930-1.prepared.out`
- `build/agent_state/442_step4_residual_disposition/930930-1.prepared.err`
- `build/agent_state/442_step4_residual_disposition/930930-1.object.out`
- `build/agent_state/442_step4_residual_disposition/930930-1.object.err`

## Suggested Next

Plan-owner should split or regenerate lifecycle state for a producer authority
packet before continuing external-linkage `930930-1` provenance publication.

Recommended next source initiative: define and publish reliable
closed-world/internal/private/no-external-caller metadata that can prove when
same-module callsites are complete for a callee formal. Only after that
authority exists should idea 442-style formal provenance publication be
extended beyond `LirFunction::is_internal`.

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
- Pointer-delta propagation remains the live candidate if Step 4 keeps this
  idea active for internal/closed-world callees: `%mr_TR - 8` still needs
  constant pointer add/sub propagation from an already proven formal pointer
  source.
- For external-linkage representatives such as `930930-1::f`, do not publish
  formal pointer provenance until the producer has reliable closed-world,
  internal/private, or no-external-caller authority.
- Do not treat the generic RV64 `unsupported_instruction_fragment` object
  diagnostic as permission to infer pointer provenance in target lowering.
- Do not accept or commit any full-suite baseline refresh that changes the
  current green baseline to the rejected `test_baseline.new.log` state.

## Proof

Step 4 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
