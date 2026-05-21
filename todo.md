Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Completed Step 2: Classify Residual Buckets for idea 295 from existing
`test_after.log`, source representatives, and generated artifacts under
`build/c_testsuite_aarch64_backend/`.

Classified current residual buckets:

- `00187`: `RUNTIME_MISMATCH`, external/libc file-call result publication.
  Source checks `fread(freddy, 1, 6, f) != 6`, but generated AArch64 calls
  `fread` and then compares `[sp, #96]` against `6` instead of the return value
  in `x0` (`00187.c.s:41-44`). The runtime consequently prints
  `couldn't read fred.txt` before otherwise reading the file successfully.
  First bad fact: a libc/external call result is not published to the scalar
  comparison consumer.
- `00174`: `RUNTIME_MISMATCH`, scalar FP expression/constant materialization.
  The expected floating outputs become zeros while integer FP comparisons,
  integer-to-FP conversion, and `sin(2)` still work. Generated AArch64 starts
  by converting/copying uninitialized `d13` for `float a = 12.34 + 56.78` and
  direct constant-expression `printf` arguments (`00174.c.s:10-28`), while
  later integer-to-FP and `sin` paths use concrete producers (`00174.c.s:290-325`).
  First bad fact: FP constants/expression results are missing or not published
  before vararg `printf` consumption.
- `00216`: `RUNTIME_NONZERO` segfault, aggregate initializer/compound
  relocation/function-pointer-table behavior. The earlier local aggregate
  address crash from ideas 374/375 is not the current first visible owner by
  this artifact. Generated data now contains plausible relocations for
  `global_wrap` and `table` (`00216.c.s:2632-2639`), but `test_multi_relocs`
  ignores the loop index and always loads/calls `table[1]`
  (`00216.c.s:1988-2000`); `test_compound_with_relocs` also rewrites loaded
  function-pointer values back to `inc_global` before each indirect call
  (`00216.c.s:1866-1887`). First bad fact is still ambiguous between aggregate
  initializer relocation publication and indirect function-pointer table
  selection, so this should not outrank the cleaner `00187` owner without a
  focused split/probe.
- `00207`: `Timeout`, quarantined dynamic stack/VLA/goto/nonleaf frame case.
  Generated `f1` performs VLA stack adjustment and writes through `[x29, #16]`
  without a visible frame-pointer setup, then calls `printf` in the same
  function (`00207.c.s:4-43`). Keep parked unless the supervisor selects a
  timeout-specific owner.
- `00200`: `Timeout`, quarantined shift/type-promotion/control expansion case.
  The generated macro expansion has many synthetic branch ladders around
  constant false predicates before the first `check` calls
  (`00200.c.s:84-130`). Keep parked unless the supervisor selects a
  timeout-specific owner.

Candidate semantic owner ranking:

1. `00187` external/libc call-result publication. It has the clearest first
   bad fact and a narrow owner: returned scalar values from libc/file calls
   must be published to the comparison/store consumer instead of reading stale
   stack storage. Existing open ideas do not exactly own this by current
   evidence; recursive/call-preservation and scalar publication ideas mention
   adjacency but do not cover libc call return-count publication.
2. `00174` scalar FP expression/constant materialization. Also tractable, but
   broader than `00187` because it may cross constant folding, FP immediate
   materialization, selected FP register publication, and vararg call
   preparation.
3. `00216` aggregate initializer/compound relocation/function-pointer-table
   behavior. High value but currently mixed; split only after deciding whether
   the first owner is relocation data, computed table indexing, or indirect
   call operand publication.
4. `00207` timeout quarantine.
5. `00200` timeout quarantine.

## Suggested Next

Proceed to Step 3 owner selection: create or select a focused owner for
`00187` external/libc call-result publication unless the supervisor chooses to
split a different non-timeout bucket. Keep `00200` and `00207` quarantined.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen ideas 374/375 from `00216`
counts alone: the current artifact does not show the old local aggregate
address first bad fact as the ranking lead. Avoid merging `00187` with scalar
FP, aggregate relocation/table, or timeout work; the top owner should be
return-value publication from external calls to scalar consumers.

## Proof

No new test run was requested or run for this classification-only packet.
Used the existing `test_after.log` from:

```sh
{ cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure; } > test_after.log 2>&1
```

The captured backend subset reports `99% tests passed, 5 tests failed out of
357`, with 352 passing tests and the five residual failures listed above.
