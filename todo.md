Status: Active
Source Idea Path: ideas/open/558_bir_call_metadata_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Inspect And Repair Call-Return Metadata

# Current Packet

## Just Finished

Step 4 - Inspect And Repair Call-Return Metadata inspection half was completed
for the selected call-return representative, `src/20050121-1.c`.

Current evidence:

- `build/agent_state/558_step4_20050121.log` and
  `build/rv64_gcc_c_torture_backend/src_20050121-1.c/case.log` both report
  `bar_float` failing in `semantic call family 'call-return semantic family'`.
- `build/c4cll --target riscv64-linux-gnu --codegen llvm
  tests/c/external/gcc_torture/src/20050121-1.c` shows the failing shape:
  `%t0 = call { float, float } (i32) @foo_float(i32 5)`,
  `%t1 = extractvalue { float, float } %t0, 0`, then
  `store float %t1, ptr %p.x`.
- `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/src/20050121-1.c` fails before producing BIR
  with the same call-return semantic family note.

Exact producer boundary:

- `BirFunctionLowerer::lower_call_inst` treats this as a metadata-rich direct
  call and passes `call.return_type` as a `LirTypeRef` to
  `lower_return_info_from_type`.
- `lower_signature_aggregate_layout` rejects metadata-bearing aggregate return
  refs that do not carry a `StructNameId`. Anonymous aggregate return refs such
  as `{ float, float }`, `{ double, double }`, and `{ i32, i32 }` therefore
  cannot use the existing rendered-layout compatibility bridge.
- Because return ABI/layout admission fails at the call, BIR never publishes
  the aggregate call-result metadata: no sret aggregate result slot, no
  `aggregate_value_aliases[%t0]`, and no subsequent lane/extractvalue source
  facts for `%t1`.

Focused BIR test gap:

- Add a metadata-rich direct call fixture for an anonymous aggregate return,
  starting with `{ float, float } @callee(i32)` on RV64, followed by
  `extractvalue` of lane `0` or `1` and a scalar store.
- The test should prove producer facts, not RV64 inference: the call lowers
  with deterministic sret aggregate result storage/ABI, the aggregate result
  name is usable by the following `LirExtractValueOp`, and the extracted scalar
  lane is published as a normal BIR value/source for the store.

## Suggested Next

Execute the repair half of Step 4.

Add the focused BIR coverage above, then repair producer-side call-return
publication for metadata-rich direct calls returning anonymous aggregate values.
The repair should admit rendered anonymous aggregate return layouts only for
real anonymous aggregate type refs, publish the aggregate result/sret metadata,
and handle the immediately-following `LirExtractValueOp` as an alias to the
returned aggregate lane/source value.

Suggested narrow RV64 representative command after repair:

- `ALLOWLIST=build/agent_state/558_step4_20050121.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`

## Watchouts

Reject downstream RV64/MIR call inference, generic local-memory routing,
runtime/intrinsic repairs, expectation rewrites, unsupported-marker changes,
allowlist edits, runtime-comparison changes, and named-case shortcuts. The
current RV64 object-route failure for `src/20000412-2.c` is downstream and is
not a reason to broaden this source idea. The runbook must cover call-return
metadata before claiming the source idea is complete.

For Step 4 repair, keep the fix in BIR semantic producer lowering. Do not
repair this by weakening structured aggregate identity generally: named
aggregate signature refs should still require `StructNameId` consistency. The
anonymous aggregate bridge should be limited to type refs with rendered
aggregate spellings and no structured name carrier.

There is no backend `LirExtractValueOp` handler today. If admitting the call
return moves the row from call-return to scalar/local-memory because of
`extractvalue`, keep that as the same producer packet only when the fix is the
direct returned-aggregate lane alias/source fact. Broader arbitrary
`insertvalue`/`extractvalue` aggregate construction is a separate route unless
needed for this call-return source contract.

## Proof

Proof logs:

- `build/agent_state/558_step4_20050121.log`
- `build/rv64_gcc_c_torture_backend/src_20050121-1.c/case.log`

Commands:

- `build/c4cll --target riscv64-linux-gnu --codegen llvm
  tests/c/external/gcc_torture/src/20050121-1.c` showed the aggregate-return
  direct call plus `extractvalue`/store shape.
- `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/src/20050121-1.c` failed with the expected
  call-return semantic family diagnostic.
- `git diff --check -- todo.md` passed.

Residual classification:

- Call-return semantic admission: still active for this representative.
- Current owner: BIR call-return producer metadata.
- Current failure: metadata-rich direct anonymous aggregate return calls reject
  the return layout before publishing aggregate result/extractvalue lane facts.
