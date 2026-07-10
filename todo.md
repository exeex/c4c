Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Selected Infrastructure Rule

# Current Packet

## Just Finished

Step 3 of `plan.md` repaired the selected RV64 prepared object-emission
traversal contract for post-helper ordinary instructions.

Changed files:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `todo.md`
- `test_after.log`

Owner reasoning: `fragment_for_prepared_variadic_va_start` returned only the
helper fragment, but the outer traversal continued; the stale helper-only text
came from the following `LoadLocalInst` being pruned as dead before prepared
publication facts were considered. `object_emission.cpp` now keeps an otherwise
unused `LoadLocalInst` in traversal when it has prepared memory-access or result
home facts, allowing the existing local-memory fragment owner to emit or fail
closed. This is a general prepared-publication rule, not testcase identity or
expectation churn.

The selected `loads_rv64_va_start_published_word_after_helper` failure is no
longer present in `test_after.log`.

## Suggested Next

Next packet should select the new first remaining focused failure in
`backend_riscv_object_emission`: the first `test_after.log` line is now
`expected prepared RV64 object path to reject`, after the va_start post-helper
load row has been repaired. Classify that later rejection-contract owner before
editing unrelated relocation, byval, local memory, sret, call-argument ordering,
or diagnostic-exactness rows.

## Watchouts

- The repair deliberately does not make unsupported or incomplete prepared load
  facts silently pass; if prepared publication facts exist, the load fragment
  owner is reached and may reject with a diagnostic.
- The focused subset still contains later failures, including direct call
  relocations, byval/local-memory/sret rows, call-argument publication ordering,
  and diagnostic exactness. Those were not absorbed into this packet.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, or baseline accounting for this slice.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still failed on later
`backend_riscv_object_emission` rows. `test_after.log` is the canonical proof
log and no longer contains the selected va_start post-helper load failure.

Supervisor acceptance:

- Matching focused before/after guard passed in non-decreasing mode:
  before 0/1, after 0/1, with the same focused CTest binary still red and no
  new failing test identity.
- The selected failure text
  `expected prepared va_start load object text layout` is present in
  `test_before.log` and absent from `test_after.log`.
