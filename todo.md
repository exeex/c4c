Status: Active
Source Idea Path: ideas/open/127_lir_structured_signature_reference_producer_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Signature Reference Producer And Consumer Surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for signature reference producer and consumer
surfaces.

`signature_text` producer surface:

- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp::build_fn_signature` is the only
  producer of function header text. It renders declaration and definition
  headers from HIR return type, params, attributes, and `fn.link_name_id` via
  `emitted_link_name`; definition text may also prepend `template-origin` and
  `spec-key` comments. Classification: final spelling plus compatibility
  payload, not a structured semantic reference carrier.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp::lower` assigns that text to
  `LirFunction.signature_text` for both extern declarations and definitions,
  then calls `populate_signature_type_refs`. Classification: producer boundary
  where a future structured signature function-reference carrier should be
  populated.
- `populate_signature_type_refs` fills `signature_return_type_ref` and
  `signature_param_type_refs` from the same HIR signature data. Classification:
  verifier mirror for type identity only; it does not carry function reference
  reachability.

`signature_text` consumer surface:

- `collect_fn_refs` scans `fn.signature_text` through `scan_refs`.
  Classification: currently the only signature-text reachability-authority
  consumer, but the inventory found no actual covered signature-embedded
  function-reference family beyond the function header's own `@name` spelling
  and template-comment/fallback payload. This should be treated as compatibility
  fallback until a real producer-carried signature reference exists.
- `scan_refs` also serves legacy string payloads from instruction operands and
  global initializer text. Classification: compatibility fallback; structured
  surfaces already exist for direct calls through
  `LirCallOp::direct_callee_link_name_id` and global initializers through
  `LirGlobal::initializer_function_link_name_ids`.
- `eliminate_dead_internals` consumes `collect_fn_refs` output by seeding
  `link_name_ids` first and fallback names second. Classification:
  reachability consumer; it is ready to merge a structured
  `LirFunction` signature carrier before fallback names.
- `src/codegen/lir/verify.cpp::function_signature_line` and the
  `verify_function_signature_*_mirror` helpers parse `signature_text`.
  Classification: verifier mirror/compatibility parser that checks structured
  type refs against final spelling; not function reachability authority.
- `src/codegen/lir/lir_printer.cpp::render_signature_with_link_name` and
  `render_fn` consume `signature_text`. Classification: printer input/final
  spelling bridge; `LirFunction::link_name_id` remains the semantic function
  identity when present.
- Focused tests in
  `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp` assert
  that the printer retains declaration/definition signature text and that the
  verifier rejects mirror drift. Classification: printer/verifier coverage, not
  signature function-reference reachability coverage.
- `tests/frontend/frontend_hir_tests.cpp` includes drift tests for function
  printer/backend identity recovery from `LinkNameId`. Classification:
  diagnostics/regression coverage for existing function identity carriers, not
  evidence of a signature-embedded function-reference carrier.

Conclusion: no actual signature-embedded function-reference family was found
beyond comments/fallback scanning of rendered text. The closest real carrier
target is a minimal `LirFunction` field such as
`signature_function_link_name_ids`, populated at the existing
`build_fn_signature`/`populate_signature_type_refs` producer boundary and
consumed by `collect_fn_refs` before `scan_refs(fn.signature_text)`.

## Suggested Next

First bounded implementation packet: add the minimal structured carrier to
`LirFunction` (`std::vector<LinkNameId> signature_function_link_name_ids` or
equivalent), populate it at the same declaration/definition lowering boundary
that assigns `signature_text`, and route `collect_fn_refs` to seed those IDs
before retaining `scan_refs(fn.signature_text)` as explicit compatibility
fallback. Keep the first packet behavior-preserving if the carrier is empty for
today's signatures.

## Watchouts

The current real structured function-reference carriers are body direct calls
and global initializer function IDs, not signatures. Function pointer parameter
or return types lower to `ptr`/type metadata and do not name callee functions in
the signature. Avoid inventing semantic references from the header's own
function symbol; that is declaration identity/final spelling, not a dependency
edge. If a later packet discovers a concrete HIR source for signature-embedded
function refs, populate the carrier from that source rather than reparsing
`signature_text`.

## Proof

Ran delegated proof:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests$|frontend_lir_|backend_)'; } 2>&1 | tee test_after.log`

Result: passed. Build reported no work to do; CTest reported 100% tests passed,
0 failed out of 113 run, with 12 disabled tests not run. Proof log:
`test_after.log`.
