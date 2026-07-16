Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed the second Step 4 residual bridge repair after the aggregate-slot
adapter cleanup series.

Call lowering aggregate-value alias layout now uses
`LocalAggregateSlots::type_ref` when present before falling back to rendered
slot text for no-id aliases. Metadata-bearing aggregate aliases passed by
address now resolve layout through structured type-ref lookup and fail closed on
misses. This follows the previous aggregate-copy repair, which moved copy-size
validation and copy-to-pointer/global leaf discovery onto the same slot-state
lookup.

Unmet Step 4 criteria:
- Residual no-id bridge comments remain outside the direct aggregate-slot
  declaration packet, including memory/provenance, memory/addressing,
  memory/intrinsics, `local_gep`, globals, call ABI raw signature/byval routes,
  aggregate helper layout, type raw lookup, and global initializers.
- These are still potentially within 847's compile-time separation and expired
  adapter deletion scope until classified with evidence as already safe,
  deleted, or owned by a separate source idea.

## Suggested Next

Suggested Next: continue the bounded Step 4 repair/classification packet.

Classify the remaining residual no-id bridge inventory by valid-LIR authority
risk, then delete or structurally route one tight family of remaining in-scope
adapters. Start with whichever remaining family has direct valid-LIR authority
impact and nearby proof available, likely memory/provenance or call ABI raw
signature/byval routes. For each residual bridge touched, prove one of:
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

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
