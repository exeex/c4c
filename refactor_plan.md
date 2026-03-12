# LLVM Const-Init / HIR Refactor Plan

## Goal

Reduce `src/codegen/llvm/hir_to_llvm_const_init.cpp` and the non-LLVM-specific parts of `src/codegen/llvm/hir_to_llvm_helpers.hpp` by moving C initializer and layout semantics into HIR lowering.

The target end state is:

- HIR owns:
  - aggregate initializer interpretation
  - brace elision handling
  - array-size deduction from initializers
  - subobject mapping for struct/array/union init
  - struct/union layout metadata
- LLVM backend owns:
  - LLVM type mapping
  - LLVM constant emission from already-resolved HIR init
  - instruction emission only


## Current State

Today the backend still does three jobs at once:

1. LLVM type lowering
2. C object layout reconstruction
3. C initializer interpretation

This is why:

- `hir_to_llvm_const_init.cpp` is large and stateful
- `hir_to_llvm_helpers.hpp` contains non-backend logic such as:
  - `sizeof_ts`
  - `compute_struct_size`
  - `compute_struct_align`
  - `compute_field_size`
  - `alignof_field_elem`
  - string escape decoding helpers reused outside true LLVM lowering

We already started moving global initializer normalization into HIR, but only for a conservative subset of plain struct brace-elision cases.


## Refactor Principles

1. Do not rewrite everything at once.
2. Prefer moving semantics forward before deleting backend code.
3. Keep `hir_emitter` behavior stable while HIR gets richer.
4. Use `--dump-hir` as the primary visibility tool for each migration step.
5. Expand normalization only when a case is clearly represented in HIR without ambiguity.


## Phase 1: Move Layout Metadata Into HIR

### Objective

Stop recomputing struct/union layout in LLVM backend.

### Changes

Extend HIR metadata so layout is computed once in frontend lowering:

- `HirStructDef`
  - add `size_bytes`
  - add `align_bytes`
- `HirStructField`
  - add `offset_bytes`
  - add `size_bytes`
  - add `align_bytes`

Existing fields that should remain:

- `llvm_idx`
- `bit_width`
- `bit_offset`
- `storage_unit_bits`
- `is_bf_signed`
- `array_first_dim`

### Implementation Notes

- Compute these where struct defs are lowered in `src/frontend/sema/ast_to_hir.cpp`.
- Keep the current bitfield packing algorithm as the source of truth.
- Treat flexible array members explicitly instead of re-deriving from `array_first_dim == 0` in backend.

### Backend Follow-up

After metadata exists, replace uses of:

- `compute_struct_size`
- `compute_struct_align`
- `compute_field_size`
- `alignof_field_elem`

with direct HIR metadata reads.

### Validation

- `--dump-hir` should print size/alignment metadata for structs and fields.
- No change in emitted LLVM IR for representative layout-heavy tests.
- Re-run at minimum:
  - `tests/c-testsuite/tests/single-exec/00216.c`
  - `tests/c-testsuite/tests/single-exec/00150.c`


## Phase 2: Move String Literal Byte Semantics Out of LLVM Helpers

### Objective

Backend should not be the owner of generic C string escape decoding.

### Changes

Extract reusable string literal decoding utilities into a frontend/common module, for example:

- `src/common/c_string_literal.hpp`
- or `src/frontend/common/string_literal.hpp`

Move or consolidate:

- byte-oriented C escape decoding
- wide-string decoding helpers, if practical

### Notes

The same semantic helper should be reused by:

- HIR array-size deduction
- HIR initializer normalization
- LLVM string constant emission

This avoids drift between frontend and backend behavior.

### Validation

Re-run at minimum:

- `tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/20000227-1.c`
- string-array initializer cases from `00216.c`


## Phase 3: Expand HIR Global Initializer Normalization

### Objective

Move more aggregate init interpretation from `hir_to_llvm_const_init.cpp` into HIR lowering.

### Scope

Do this incrementally.

Recommended order:

1. plain struct brace elision
2. plain array brace elision
3. string-to-char-array initialization
4. designated array init
5. designated struct init
6. union active-field selection
7. flexible-array initialization

### Important Constraint

Do not use a purely syntactic flattening pass.

Normalization must remain typed:

- each child init must correspond to a concrete subobject
- brace elision must be resolved against target type
- omitted fields/elements must be distinguishable from explicit zero

### Suggested Representation Strategy

Short term:

- keep `GlobalInit`
- make its shape more normalized before codegen

Longer term:

- add a resolved form separate from raw `InitList`, for example:
  - `ResolvedInitScalar`
  - `ResolvedInitArray`
  - `ResolvedInitStruct`
  - `ResolvedInitUnion`

or a generic typed tree:

- `ResolvedInitNode { TypeSpec ts; slots...; scalar... }`

### Validation

Every time support is expanded, add or reuse cases covering:

- brace elision
- nested designators
- array-of-struct init
- struct-with-array-field init
- union init
- flexible array member init

Minimum existing regression set:

- `tests/c-testsuite/tests/single-exec/00150.c`
- `tests/c-testsuite/tests/single-exec/00216.c`
- `tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/20000227-1.c`


## Phase 4: Slim Down `hir_to_llvm_const_init.cpp`

### Objective

Once HIR carries resolved initializer meaning, backend should emit constants, not infer semantics.

### Code To Remove or Reduce

These categories should disappear from LLVM backend:

- brace-elision interpretation
- flat scalar consumption logic
- array-size deduction from initializer
- subobject targeting decisions
- union field selection from raw init syntax

Concrete examples currently living in backend:

- `consume_from_flat`-style logic
- `needs_brace_elision`
- `flat_scalar_count`
- `deduce_array_size_from_init`

### Desired End State

`hir_to_llvm_const_init.cpp` should mostly become:

- walk normalized init tree
- emit LLVM literals for:
  - scalar
  - array
  - struct
  - union


## Phase 5: Trim `hir_to_llvm_helpers.hpp`

### Keep In Backend

These are legitimate LLVM/backend helpers:

- `llvm_ty`
- `llvm_alloca_ty`
- `llvm_field_ty`
- `sanitize_llvm_ident`
- floating literal formatting helpers

### Move Out

These should no longer live in LLVM helper layer:

- `sizeof_ts`
- `compute_struct_size`
- `compute_struct_align`
- `compute_field_size`
- `alignof_field_elem`
- generic string escape decoding helpers

### Possible Result

Split the current file into:

- `llvm_type_helpers.hpp`
- `llvm_literal_helpers.hpp`

and delete the layout/initializer-semantics half entirely.


## Suggested Execution Order

1. Phase 1: layout metadata in HIR
2. Phase 2: shared string literal decoding helpers
3. Phase 3: continue HIR global-init normalization
4. Phase 4: remove backend initializer interpretation incrementally
5. Phase 5: split/shrink `hir_to_llvm_helpers.hpp`

This order is intentional:

- layout metadata is the safest migration
- string decoding unifies semantics early
- only then is it safe to remove backend init logic


## Practical Work Breakdown

### Step A

Add HIR layout metadata and print it in `--dump-hir`.

### Step B

Replace backend layout helper call sites with HIR metadata reads.

### Step C

Move shared string decode helpers out of LLVM code.

### Step D

Choose one initializer family at a time and migrate it:

- struct brace elision
- array brace elision
- string arrays
- designators
- unions

### Step E

Delete backend logic only after HIR dump clearly shows the resolved shape.


## Verification Strategy

For each migration step:

1. build `c4cll`
2. inspect `--dump-hir` on focused cases
3. compile to LLVM IR
4. where possible, run the produced binary and compare against host `clang`

Recommended focused tests:

- `tests/c-testsuite/tests/single-exec/00150.c`
- `tests/c-testsuite/tests/single-exec/00216.c`
- `tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/20000227-1.c`

If doing a broader gate later, use the regression-guard skill and compare before/after logs.


## Immediate Next Step

Start with Phase 1.

Reason:

- it removes clearly misplaced backend responsibilities
- it is mechanically safer than expanding initializer normalization
- it creates the metadata foundation needed for the later cleanup
