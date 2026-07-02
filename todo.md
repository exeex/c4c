Status: Active
Source Idea Path: ideas/open/558_bir_call_metadata_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reconcile Call Metadata Representatives

# Current Packet

## Just Finished

Step 4 - Inspect And Repair Call-Return Metadata repair was completed for the
selected call-return representative, `src/20050121-1.c`.

Implemented repair:

- `lower_signature_aggregate_layout` now admits metadata-rich anonymous
  aggregate return type refs whose rendered spelling is a real anonymous
  aggregate (`{ ... }` or `[ ... ]`) while preserving fail-closed behavior for
  named aggregate refs without `StructNameId`.
- `lower_scalar_or_local_memory_inst` now handles `LirExtractValueOp` only when
  the aggregate operand is a known local aggregate alias, loading the requested
  scalar leaf slot into the extract result.
- `backend_lir_to_bir_notes_test` now covers an RV64 metadata-rich direct call
  returning `{ float, float }`, followed by `extractvalue` and a scalar store,
  and checks the sret call metadata, lane load, and stored source value.

Representative result:

- `src/20050121-1.c` no longer reports `semantic call family 'call-return
  semantic family'`.
- The row now reaches downstream RV64 object-route unsupported ownership:
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

## Suggested Next

Step 5 - Reconcile Call Metadata Representatives should rerun and record both
representative rows:

- direct-call: `src/20000412-2.c`
- call-return: `src/20050121-1.c`

Compare current `case.log` outcomes against the original direct-call and
call-return semantic metadata families, confirm there were no expectation,
unsupported-marker, allowlist, runtime-comparison, or semantic admission
weakening changes, and decide whether the source idea should close, split a
distinct downstream initiative, or continue with another call-metadata
checkpoint.

## Watchouts

Reject downstream RV64/MIR call inference, generic local-memory routing,
runtime/intrinsic repairs, expectation rewrites, unsupported-marker changes,
allowlist edits, runtime-comparison changes, and named-case shortcuts. The
current RV64 object-route failure for `src/20050121-1.c` is downstream and is
not a reason to broaden this source idea. The runbook must not claim
runtime/object lowering progress from this packet.

The new `LirExtractValueOp` lowering is intentionally narrow: it requires a
known local aggregate alias and publishes a scalar leaf load. Broader arbitrary
`insertvalue`/`extractvalue` aggregate construction remains separate unless a
future call-return source contract directly requires it.

## Proof

Proof logs:

- `test_after.log`
- `build/agent_state/558_step4_20050121_after.log`
- `build/rv64_gcc_c_torture_backend/src_20050121-1.c/case.log`

Commands:

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log` passed: 345/345 backend
  tests.
- `ALLOWLIST=build/agent_state/558_step4_20050121.allowlist
  VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh >
  build/agent_state/558_step4_20050121_after.log 2>&1` exited 1 only because
  the representative moved to downstream RV64 object-route unsupported
  ownership.

Residual classification:

- Call-return semantic admission: repaired for this representative.
- Current owner: downstream RV64 object lowering.
- Current failure: unsupported BIR instruction in RV64 object route, not
  producer-side call-return metadata.
