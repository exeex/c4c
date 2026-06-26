# RV64 Global Aggregate Lane Materialization

Status: Open
Type: Target object-route follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/382_rv64_object_route_prepared_local_memory_addressing.md`
Related: `ideas/closed/357_rv64_object_route_data_sections_globals_strings.md`

## Goal

Route or lower RV64 object-route global aggregate lane loads when prepared BIR
represents global-source data as `load_local ... addr <global>` without the
frame-slot or pointer-value local-memory facts required by the local-memory
object route.

## Why This Exists

Closing idea 382 moved `src/20030914-2.c` past the prepared byval local-memory
lane loads. The representative now fails first in `main`, block `entry`, at
global aggregate lanes:

```text
%t0.global.aggregate.load.0 = bir.load_local i32 %t0.0, addr gs
```

The same pattern repeats through offset `68`. The prepared-addressing section
publishes frame-slot stores for the aggregate destination, but it does not
publish frame-slot or pointer-value access metadata for the global-source
loads. This is not another prepared local-memory base shape. It is a
global/data materialization contract boundary exposed behind the shared local
memory diagnostic.

Idea 357 is already closed for prepared globals, strings, ELF data sections,
symbols, and relocations that the prepared module contract already publishes.
This follow-up owns the narrower residual shape where aggregate global data is
presented to target object emission as lane loads from a global address without
prepared global memory facts suitable for semantic lowering.

## In Scope

- Audit the prepared BIR and module metadata for `load_local ... addr gs`
  global aggregate lanes in `src/20030914-2.c`.
- Determine whether the missing fact belongs in upstream prepared-data
  publication or in RV64 object-route consumption of already-published global
  aggregate data.
- Add or extend focused tests for global aggregate lane materialization from
  explicit prepared facts.
- If prepared data facts are already complete, lower the RV64 object-route
  lane loads through data symbols, relocations, or supported global memory
  access helpers.
- If facts are missing, preserve a precise unsupported diagnostic and route the
  gap to the correct upstream data-publication owner instead of inferring
  initializer bytes or aggregate layout in target emission.
- Rerun `src/20030914-2.c` and document the next owner when this boundary
  advances.

## Out of Scope

- Reopening prepared frame-slot or pointer-value local-memory addressing; idea
  382 closed that boundary.
- Reopening blanket prepared module globals, strings, ELF data sections,
  symbols, or relocations already closed by idea 357.
- Inferring aggregate initializer bytes, symbol identity, storage duration, or
  address-use semantics from C source syntax, testcase names, raw object
  offsets, or log text.
- Treating the global-source lanes as stack local memory merely to satisfy the
  existing local-memory diagnostic.
- Aggregate `va_arg` helper lowering; use
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.

## Acceptance Criteria

- Focused backend tests prove the first supported global aggregate lane shape
  or prove a narrower unsupported upstream-publication boundary.
- `src/20030914-2.c` advances past the `main` `addr gs` aggregate lane loads
  because of semantic global/data materialization, or records a precise
  upstream prepared-data owner.
- Existing RV64 object-emission and object/data tests remain green.
- Any emitted global aggregate data uses explicit prepared facts and valid RV64
  object model section, symbol, relocation, and address materialization
  semantics.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/20030914-2.c` or global `gs`.
- Reject treating `load_local ... addr <global>` as stack local memory without
  prepared global memory facts.
- Reject reconstructing aggregate byte ranges, initializer bytes, symbol
  identity, or relocation semantics from source syntax, raw offsets, or log
  text not published by prepared metadata.
- Reject expectation downgrades, allowlist filtering, diagnostic-only renames,
  or skip broadening claimed as global aggregate materialization progress.
- Reject reopening broad idea 357 scope unless the route proves the prepared
  module already publishes complete data facts and only target object
  consumption remains.
- Reject retaining the exact `main` `addr gs` lane failure behind a renamed
  helper or less precise diagnostic.
