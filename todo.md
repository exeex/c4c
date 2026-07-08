Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh residual diagnostics

# Current Packet

## Just Finished

- Step 1 refreshed the narrowed residual diagnostics for `src/pr36034-1.c` and
  `src/pr91137.c`.
- Both rows still fail RV64 object compilation at the first prepared/global
  authority gate with:
  `unsupported_global_data: RV64 object route requires supported prepared global
  memory facts`.
- `src/pr36034-1.c`: prepared addressing already has direct
  `base=global_symbol` rows for `x` and `tmp` with concrete offsets, 8-byte
  widths, alignment, base-plus-offset form, and in-bounds ranges, but the
  global-memory publication authority remains incomplete because those rows
  carry `layout_authority=unknown`.
- `src/pr91137.c`: prepared addressing already has direct
  `base=global_symbol` rows over `b`, `c`, `d`, `e`, and `a`; the representative
  aggregate rows for `c`/`d` carry 4-byte widths, concrete offsets,
  `layout_authority=byte_storage_aggregate`, and in-bounds ranges, while scalar
  rows such as `b`/`e`/`a` carry scalar layout. It still reaches the same
  supported prepared global-memory fact gate, so the missing producer fact is
  publication/admission of these supported prepared global-memory accesses, not
  object-data emission.
- First owner for both rows is prepared/prealloc global-memory fact publication
  and verification. They are not selected object-data residuals and not RV64
  global load/store consumer failures: the RV64 consumer sees prepared memory
  rows and stops only because the producer-side supported global-memory
  authority predicate is not satisfied.

## Suggested Next

- Step 2 implementation packet: repair the shared producer-side prepared
  global-memory publication rule so direct global-symbol prepared memory
  accesses are marked supported only when the prepared address proves global
  object identity, concrete offset, supported width/alignment, complete extent,
  and an in-bounds requested range. Start with the existing authority predicate
  around `prepared_global_symbol_memory_has_publication_authority(...)` and the
  global load/store publication/admission paths, preserving fail-closed behavior
  for unknown layout authority, missing extent, ambiguous identity, unsupported
  width, non-global storage, and unsupported sections.
- Exact Step 2 proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- Do not close idea `608` until the residual prepared global-memory evidence
  has moved or been explicitly re-owned.
- Do not route `src/pr36034-1.c` or `src/pr91137.c` through selected
  object-data authority; current diagnostics confirm the first stop is prepared
  global-memory fact publication.
- Keep RV64 emission, relocation records, unsupported markers, allowlists,
  timeout/accounting files, and expectations out of this runbook.
- Avoid a testcase-shaped exception for `x`, `tmp`, `c`, or `d`; the next slice
  must publish a general supported global-memory fact or fail closed on a named
  missing prerequisite.

## Proof

- Focused residual probe:
  `ctest --test-dir build --output-on-failure -R
  '^(llvm_gcc_c_torture_src_pr36034_1_c|llvm_gcc_c_torture_src_pr91137_c)$'`
  passed, confirming the front-end torture rows still compile while RV64 object
  compilation remains the prepared/global authority residual.
- Delegated proof run:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Result: passed. Proof log path: `test_after.log`.
