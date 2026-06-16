Status: Active
Source Idea Path: ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and classify suffix parser use

# Current Packet

## Just Finished

Step 1 from `plan.md` completed: inventoried every current
`strip_call_arg_abi_type_suffix` declaration, definition, and call site, plus
the visible rendered `alignstack(...)` emitters/fixtures.

Inventory and classifications:

- `src/backend/bir/lir_to_bir/calling.cpp:684`: only definition; no separate
  declaration found in `src`, `tests`, `include`, or `docs`. Internal unnamed
  namespace helper that strips a rendered trailing ` alignstack(...)` suffix.
  Classification: compatibility parser targeted for quarantine/removal.
- `src/backend/bir/lir_to_bir/calling.cpp:1389`: `lower_byval_call_arg_layout`
  branch for non-empty `structured_args` with empty `LirCallArg::type_ref`.
  Classification: raw/no-ref legacy compatibility bridge, despite positional
  structured args being present; should remain fenced or be removed when all
  structured args carry refs.
- `src/backend/bir/lir_to_bir/calling.cpp:1394`: structured-args branch without
  `StructNameId`, stripping legacy rendered type for comparison.
  Classification: structured call-argument path that should not parse rendered
  suffixes for semantic authority.
- `src/backend/bir/lir_to_bir/calling.cpp:1395`: same branch, stripping
  `LirCallArg::type_ref.str()` for comparison.
  Classification: structured call-argument path that currently lets a rendered
  suffix participate in type identity; should route through structured metadata.
- `src/backend/bir/lir_to_bir/calling.cpp:1407`: structured-args branch with
  `StructNameId`, stripping legacy rendered text only when the byval pointee
  parser did not produce a type.
  Classification: structured path guard; provisional target to avoid rendered
  suffix parsing when a structured name id exists.
- `src/backend/bir/lir_to_bir/calling.cpp:1438`: no `structured_args` and empty
  `arg_type_refs` branch.
  Classification: raw/no-ref legacy compatibility path for hand-built LIR
  calls; acceptable only behind an explicit legacy/no-ref fence.
- `src/backend/bir/lir_to_bir/calling.cpp:1453`: `arg_type_refs` branch with
  `StructNameId`, stripping legacy rendered text only if byval pointee parsing
  fails.
  Classification: structured mirror path guard; provisional target to avoid
  rendered suffix parsing once metadata authority is available.
- `src/backend/bir/lir_to_bir/calling.cpp:1747`: direct-global typed-call
  scalar/function-pointer classification strips suffix from parsed parameter
  text before falling through to aggregate lowering.
  Classification: structured semantic lowering path when structured metadata is
  present, otherwise legacy parser front door; needs routing so structured
  aggregate/scalar decisions do not depend on rendered suffix text.
- `src/backend/bir/lir_to_bir/calling.cpp:1849`: generic typed-call
  scalar/function-pointer classification strips suffix from parsed parameter
  text before falling through to aggregate lowering.
  Classification: same as `1747` for the non-direct parsed-call route.

Related `alignstack(...)` sites:

- `src/codegen/lir/call_args.hpp:307`: formatter appends rendered
  `alignstack(...)` from `OwnedLirTypedCallArg::aarch64_stack_align_bytes`.
  Classification: presentation/legacy text emission, not semantic owner.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp:4248` and `:4250`:
  fixture provides both rendered suffix text and structured
  `LirCallArg::aarch64_stack_align_bytes = 8`.
  Classification: test-only fixture coverage for structured carrier behavior;
  useful for proving structured metadata wins over stale rendered text.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp:4343` and `:4345`:
  fail-closed fixture also provides rendered suffix text and structured
  `aarch64_stack_align_bytes = 8`.
  Classification: test-only fixture coverage for mismatch/fail-closed behavior.

Structured metadata owner candidate:

- Use `LirCallArg::aarch64_stack_align_bytes` as the supported call-argument
  `alignstack` authority. It is already carried from
  `OwnedLirTypedCallArg::aarch64_stack_align_bytes` through
  `lir_call_structured_args`, consumed by `apply_call_arg_metadata`, and used by
  AArch64 variadic HFA carrier lane metadata. `LirTypeRef`/`arg_type_refs`
  should remain type identity carriers, not the alignment owner.

## Suggested Next

Execute Step 2 from `plan.md`: route structured lowering through
`LirCallArg::aarch64_stack_align_bytes` and type identity metadata, starting
with `lower_byval_call_arg_layout` structured branches at
`calling.cpp:1375-1453` and the scalar pre-classification call sites at
`calling.cpp:1747` and `calling.cpp:1849`. Keep `calling.cpp:1438` as the
initial legacy/no-ref quarantine candidate unless the implementation proves it
can be removed.

## Watchouts

`arg_type_refs` may contain rendered suffix text in existing fixtures, but it
does not carry the dedicated alignment field. Avoid making `LirTypeRef::str()`
the long-term `alignstack` authority. The likely production metadata path is
`OwnedLirTypedCallArg::aarch64_stack_align_bytes` ->
`LirCallArg::aarch64_stack_align_bytes`; raw/no-ref compatibility should be
explicitly fenced and should not rescue structured metadata mismatches.

## Proof

Ran:

`rg -n "strip_call_arg_abi_type_suffix|alignstack" src tests include docs 2>/dev/null || true`

Result: command completed and output was preserved in `test_after.log`.
Sufficient for this inventory-only packet; no build or tests were run.
