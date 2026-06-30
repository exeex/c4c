Status: Active
Source Idea Path: ideas/open/429_rv64_pointer_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Lower Coherent Pointer Cast Movement

# Current Packet

## Just Finished

Completed Step 1 classification for
`ideas/open/429_rv64_pointer_address_materialization.md`.

Probe artifacts are under
`build/agent_state/429_step1_pointer_address_classification/`:
`probe_summary.tsv`, `snippets.txt`, per-row prepared/BIR/object logs, and
`classification.md`.

Representative accepted evidence:

- `src/20000622-1.c`: coherent prepared `%t2 = bir.inttoptr i32 12 to ptr`
  with GPR result home `t0`; accepted as the clearest first-packet row.
- `src/20010329-1.c`: coherent prepared `inttoptr` rematerializable-immediate
  forms with GPR result homes, including `%t1`, `%t4`, `%t7`, `%t31`, and
  `%t45`; stack-slot result form `%t17` remains outside the first packet.
- `src/20011019-1.c`: coherent `%t3 = bir.ptrtoint ptr %t0 to i64` with GPR
  source/result homes, but embedded in global load/store addressing; use as
  focused `ptrtoint` shape evidence only, not as full-row acceptance.

Rejected or out-of-scope representative row classes:

- `src/930930-1.c`: `inttoptr` fragments exist, but the representative is
  coupled to stack-slot cast results, pointer-base-plus-offset memory facts,
  pointer-value loads/stores, and a global store fact for `mem`.
- `src/20041112-1.c`: integer-to-pointer sentinel forms exist, but the row also
  returns `ptr @global` and uses global-symbol access facts; this is later
  address/global materialization work, not the first packet.
- Global symbol address materialization, relocation/addressability inference,
  pointer-base-plus-offset memory addressing, stack-slot pointer-cast result
  publication, missing-provenance/object facts, aggregate ABI, call/select,
  F128, FP, expectations, allowlists, unsupported markers, runtime comparison,
  and pass/fail accounting are out of scope for the first implementation
  packet.

Chosen first implementation target: Step 2, lower coherent prepared scalar
pointer-cast movement in the RV64 object route for authoritative prepared
`inttoptr`/`ptrtoint` facts with scalar GPR-compatible source/result homes and
rematerializable integer immediates. Do not infer object identity from integer
shape or representative filenames.

Primary files for the next packet: `src/backend/mir/riscv/codegen/object_emission.cpp`,
`src/backend/mir/riscv/codegen/prepared_function_emit.cpp`,
`tests/backend/mir/backend_riscv_object_emission_test.cpp`, `todo.md`, and
`test_after.log`.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

## Suggested Next

Execute Step 2: implement coherent prepared scalar pointer-cast movement for
GPR-compatible `inttoptr`/`ptrtoint` facts in the RV64 object route, with
focused fail-closed tests for missing homes, stack-result/global/address
residuals, and non-scalar/out-of-scope forms.

## Watchouts

- Do not infer object identity, provenance, relocation base, or addressability
  from integer shape, source filenames, raw BIR, or representative row text.
- Keep global memory/addressing residual rows separate unless prepared facts
  prove they are ordinary address materialization within this idea.
- If the first packet exposes a BIR/prepared provenance gap, stop and request a
  lifecycle split instead of patching RV64 lowering.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 1 classification-only check:

```sh
git diff --check
```
