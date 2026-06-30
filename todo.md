Status: Active
Source Idea Path: ideas/open/434_bir_call_arg_abi_scalar_metadata_coherence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close-Readiness Review

# Current Packet

## Just Finished

Completed Step 3: repaired raw call argument ABI production so scalar call
arguments no longer publish aggregate-only `sret_pointer` or `byval_copy`
metadata.

Implementation summary:

- `calling.cpp` now accepts structured scalar call-argument type spellings with
  stale scalar `byval(...)` or `sret(...)` suffixes, while preserving pointer
  aggregate `ptr byval(...)` / `ptr sret(...)` carriers as aggregate metadata.
- `calling.cpp` also rewrites any scalar non-pointer argument ABI that still
  carries aggregate-only flags from target scalar call-argument ABI facts before
  raw BIR publication.
- `call_abi.cpp` now publishes `i16` as a normal scalar integer call argument.
- Focused structured-context coverage proves raw and prepared BIR drop stale
  scalar `i16 byval(...)` / `i16 sret(...)` suffixes while existing aggregate
  structured-context coverage continues to preserve pointer aggregate metadata.
- Representative probes for `20010224-1.c` and `20020506-1.c` now print plain
  `i16` scalar call arguments in raw and prepared BIR; `rg -n
  "i16 (byval|sret)\(" build/agent_state/434_step3_scalar_arg_abi_repair/*.after.dump_*`
  has no matches.
- Lookup/probe summary artifacts for this packet are under
  `build/agent_state/434_step3_scalar_arg_abi_repair/`.

## Suggested Next

Execute Step 4: Close-Readiness Review.

Recommended scope: validate the source idea acceptance criteria, confirm the
representative scalar ABI residuals are gone from raw/prepared BIR, and route
any remaining non-scalar or aggregate ABI residuals separately.

Recommended proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not implement RV64 aggregate lowering in this plan.
- Do not special-case representative testcase names, function names, or exact
  bogus alignment literals.
- Preserve coherent aggregate `sret`/`byval` pointer metadata.
- Step 3 repaired scalar non-pointer call arguments only; keep aggregate ABI
  lowering and any non-scalar residuals out of this source idea unless Step 4
  classifies them as part of the scalar metadata contract.
- Keep F128, long-double, runtime-helper, inline-asm, select, store-publication,
  expectations, allowlists, unsupported markers, runtime comparison, and
  pass/fail accounting out of this packet.

## Proof

Focused pre-proof:

```sh
cmake --build build -j2 --target backend_prepare_structured_context_test && ctest --test-dir build -j2 --output-on-failure -R '^backend_prepare_structured_context$'
```

Delegated proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
