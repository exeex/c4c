Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed the Step 4 aggregate-slot adapter cleanup series, but Step 4 is not
complete enough to hand off to Step 5 yet.

Accepted progress in this series:
- PHI aggregate slot declarations now carry
  `PhiLoweringPlan::boundary_type_ref`.
- `LocalAggregateSlots` has optional `LirTypeRef` identity.
- `declare_local_aggregate_slots(...)` has structured-ref overloads;
  metadata-bearing refs prefer structured layout/fail closed, and absent/no-id
  paths retain the explicit text fallback.
- Aggregate alloca, aggregate load result, variadic aggregate `va_arg`,
  aggregate parameter alias, and call-return sret slot declarations now pass
  available structured refs into local aggregate slot state.
- Matching backend before/after guards passed for each code slice.

Unmet Step 4 criteria:
- Residual no-id bridge comments remain outside the direct aggregate-slot
  declaration packet, including memory/provenance, memory/addressing,
  memory/intrinsics, `local_gep`, globals, call ABI raw signature/byval routes,
  aggregate helper/copy layout, type raw lookup, and global initializers.
- These are still potentially within 847's compile-time separation and expired
  adapter deletion scope until classified with evidence as already safe,
  deleted, or owned by a separate source idea.

## Suggested Next

Suggested Next: bounded Step 4 repair packet.

Classify the residual no-id bridge inventory by valid-LIR authority risk, then
delete or structurally route one tight family of remaining in-scope adapters.
Start with whichever family has direct valid-LIR authority impact and nearby
proof available, likely memory/provenance or call ABI raw signature/byval
routes. For each residual bridge touched, prove one of:
- structured/native facts now replace the bridge and the adapter can be deleted;
- the path is a deliberate no-id fallback with no semantic authority and should
  be documented in `todo.md` only;
- the path belongs to a separately scoped blocker/successor and needs
  plan-owner lifecycle representation before execution continues.

## Watchouts

- Do not advance to Step 5 while residual no-id bridge comments that may be
  valid-LIR semantic authority remain unclassified.
- Do not silently expand 847 into unrelated owners. If a residual bridge is not
  an 847 deletion target, preserve the current Step 4 return point and ask
  plan-owner to create or switch to the correct separate initiative.
- Keep legitimate no-id text fallbacks explicit and narrow; do not replace the
  deleted universal model with a renamed compatibility bag.

## Proof

Latest supplemental proof after the aggregate-slot series:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|frontend_lir_|verify_tests_)'; } > test_after.log 2>&1`

Result: passed, 18/18.
