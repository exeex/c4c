Status: Active
Source Idea Path: ideas/open/442_pointer_value_memory_provenance_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Producer Publication Or Policy Coverage

# Current Packet

## Just Finished

Completed corrective Step 3: implemented `Publish Same-Module
Computed-Address Formal Provenance` only for coherent same-module pointer
formals proven not externally callable.

Implementation:

- Added a BIR producer pass in `src/backend/bir/lir_to_bir/module.cpp` that
  collects direct same-module callsites and admits a pointer formal only when
  the callee is proven internal from `LirFunction::is_internal` and every
  relevant callsite publishes one coherent computed-address argument source.
- The accepted shape is restricted to non-sret, non-byval pointer formals with
  a concrete `@global + source_delta` argument whose global object has known
  linear extent.
- The pass tracks the callee formal through direct local-slot store/load
  publication and updates pointer-value memory access provenance with a
  concrete global-symbol subobject identity, complete remaining extent,
  `ScalarLayout`, and `ProvenInBounds`.
- No RV64 target lowering, expectations, allowlists, unsupported markers, or
  pass/fail accounting were changed.
- The previous overbroad same-module rule for externally callable functions
  was rejected: observed same-module callsites are not complete authority for
  external-linkage definitions.

Focused coverage:

- `tests/backend/bir/backend_prepare_stack_layout_test.cpp` now builds a
  semantic LIR fixture with a caller passing `@same_module_mem + 8` to an
  internal same-module pointer formal, then checks three callee
  local-store/reload pointer-value loads classify true under
  `prepare::prepared_pointer_value_memory_has_proven_authority`.
- The same fixture with external-linkage callee remains fail-closed under the
  authority classifier.

Representative probe:

- `930930-1` arg3 still publishes `source_base=@mem source_delta=792`, but
  `f` is external-linkage, so same-module callsites are not complete authority.
- The three `%lv.param.reg1` loads at `f:block_4:inst1`,
  `f:logic.rhs.11:inst1`, and `f:block_5:inst1` remain
  `layout_authority=unknown range_verdict=unknown_compatible`.
- The split pointer-delta row `f:block_5:inst5` through
  `%t27 = %t26 - 8` also remains
  `layout_authority=unknown range_verdict=unknown_compatible`.

Artifacts:

- `build/agent_state/442_step3_concrete_call_arg_provenance/summary.md`
- `build/agent_state/442_step3_concrete_call_arg_provenance/930930-1.prepared.out`
- `build/agent_state/442_step3_concrete_call_arg_provenance/930930-1.prepared.err`

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

Recommended Step 4 packet: re-probe `930930-1` pointer-value memory residuals
and decide whether idea 442 should route a closed-world/internal-linkage
metadata producer split before any external-linkage representative provenance
publication. Pointer-delta propagation remains a later packet only after the
base formal provenance is soundly authorized.

Suggested owned files:

- `todo.md`
- `test_after.log`
- `build/agent_state/442_step4_residual_disposition/*`

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

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

## Proof

Step 3 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
