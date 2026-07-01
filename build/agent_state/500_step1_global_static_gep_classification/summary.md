# 500 Step 1 Global/Static GEP Classification

Step 1 reproduced the six `global_or_static_object_gep_boundary` rows split out
by idea 499 and classified them for a focused global/static GEP admission
contract.

## Evidence Sources

- Parent classification:
  `build/agent_state/499_step1_gep_local_memory_classification/rows.tsv`
- Current scan logs:
  `build/rv64_gcc_c_torture_backend/*/case.log`
- Source snippets:
  `tests/c/external/gcc_torture/src/20000717-4.c`,
  `tests/c/external/gcc_torture/src/20031214-1.c`,
  `tests/c/external/gcc_torture/src/20051104-1.c`,
  `tests/c/external/gcc_torture/src/20080424-1.c`,
  `tests/c/external/gcc_torture/src/20120808-1.c`, and
  `tests/c/external/gcc_torture/src/ieee/copysign2.c`.

## Reproduced Rows

The row set contains exactly six entries. See `rows.tsv` for the durable row
list and `classification_counts.tsv` for counts.

| Classification | Count | Disposition |
| --- | ---: | --- |
| `direct_global_static_object_gep_candidate` | 4 | Candidate for the next global/static GEP admission contract. |
| `global_pointer_field_string_literal_boundary` | 1 | Fail closed or route to string/global-pointer provenance first. |
| `static_array_runtime_intrinsic_boundary` | 1 | Fail closed or route to runtime/string intrinsic provenance first. |

## Available Candidate Shapes

- `src/20000717-4.c`: global aggregate path
  `s.slot[0].field[!toggle]`.
- `src/20031214-1.c`: global struct field array path `g.n[j]`.
- `src/20080424-1.c`: global multidimensional array `g` passed through an
  inlined helper and indexed as `x[k]` / `x[k - 8]`.
- `src/20120808-1.c`: global byte array `d[32]` with pointer derivation
  `p = d + i`, looped preincrement, and `cp = p`.

These are candidates only when the implementation can publish explicit global
object identity, layout/range, derivation/provenance, dynamic-index authority,
and LIR producer coordinate identity. They must not be admitted from raw source
shape, testcase names, final homes, object relocations, target behavior, or
route-local slots.

## Fail-Closed Boundary Shapes

- `src/20051104-1.c`: the indexed address is the value stored in global pointer
  field `s.name` after assignment from a string literal. A direct global-object
  GEP contract is not enough; it needs string/global-pointer provenance for the
  pointed object and range.
- `src/ieee/copysign2.c`: static arrays are consumed by a copysign/memcmp test
  harness. Admission needs runtime/string intrinsic consumer authority before a
  global/static GEP consumer can be considered available.

## Existing Authority Surfaces

- `bir::Global` records global object identity, type, scalar/integer-array
  layout flags, size, alignment, initializer values, initializer symbols, and
  address materialization policy.
- `bir::LoadGlobalInst` / `bir::StoreGlobalInst` carry global name/link id,
  byte offset, alignment, and optional structured memory address.
- `BirFunctionLowerer::GlobalAddress`, `GlobalPointerMap`, and related
  provenance helpers in `src/backend/bir/lir_to_bir/memory/provenance.cpp`
  already resolve some scalar global loads/stores and linear addressed global
  accesses.
- Existing local-array semantic GEP records are local-object only; they
  deliberately report `GlobalSourceObject` for global/static rows.

## Missing Producer Surface

No durable production certificate currently ties a global/static object GEP
candidate to a matching global object, aggregate/array layout path, dynamic
index/range proof, element byte range, and LIR producer coordinate. That is the
first contract gap for the four direct candidate rows.

## First Owner

Step 2 should define the semantic global/static GEP admission contract for the
four direct candidate rows. It should also name fail-closed statuses for the
string/global-pointer and runtime/string intrinsic boundary rows instead of
folding them into the direct contract.
