# Current Packet

Status: Active
Source Idea Path: ideas/open/367_semantic_bir_indirect_local_memory_lvalue_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Indirect Local-Memory Rejection

## Just Finished

Step 1 localized the indirect local-memory lvalue rejection. Both representatives
enter the semantic local-memory coordinator in
`BirFunctionLowerer::lower_scalar_or_local_memory_inst`, then fail under
`lower_memory_store_inst` / `lower_memory_load_inst` after the provenance-first
local-memory path in `src/backend/bir/lir_to_bir/memory/local_slots.cpp`.

Concrete owner names:

- `00005.c` first bad diagnostic: `semantic lir_to_bir function 'main' failed
  in store local-memory semantic family` at `**pp = 1`.
- `00005.c` semantic owner: `try_lower_pointer_provenance_store` ->
  `try_lower_addressed_pointer_store` in
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`. A focused `/tmp` probe
  shows direct `*p = 1` lowers, and `q = *pp; *q = 1` lowers, but direct
  `**pp = 1` rejects. The boundary is the addressed-pointer scratch slot
  created from the pointer-valued load result (`result + ".addr"`) colliding
  with the store-through-pointer scratch slot (`ptr_name + ".addr"`) at a
  different type.
- `00217.c` first bad diagnostic: `semantic lir_to_bir function 'main' failed
  in load local-memory semantic family` at the read side of
  `*(unsigned*)(data + r) += a - b`.
- `00217.c` semantic owner: `try_lower_pointer_provenance_load` ->
  `try_lower_addressed_pointer_load` ->
  `can_address_scalar_subobject` in
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`. Focused `/tmp` probes
  show both `*(unsigned*)data` and `*(unsigned*)(data + r)` reject as
  load/store local-memory because the byte-pointer/global-string provenance is
  recorded with `i8`-shaped type text and the scalar subobject check does not
  admit the `i32` access through that casted byte address.

They share the same semantic owner area, the provenance-backed local-memory
load/store admission helpers in `memory/provenance.cpp`, but they are distinct
leaf branches. No lifecycle split is required before implementation if the
next packet explicitly handles both leaves under the indirect local-memory
lvalue admission rule.

## Suggested Next

Executor should start Step 2 by adding focused semantic/backend-route coverage
for both leaves: addressed-pointer store after a pointer-valued load without
scratch-slot collision, and casted byte-pointer `i32` load/store through a
local pointer value.

## Watchouts

- Keep AArch64/runtime/runner/expectation changes out of scope.
- Do not treat the current local backend-route snippet drift as progress for
  this semantic admission owner.
- Preserve the completed `00173` pointer-derived load/publication path.
- `00173` remained passing in the Step 1 proof.
- The `00005` fix should not be a `**pp` or temporary-name special case; the
  issue is reusable scratch-slot ownership in `try_lower_addressed_pointer_store`.
- The `00217` fix should not be filename- or global-string-shaped; it should
  define the semantic rule for scalar access through casted byte-address
  provenance.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00005|00173|00217)_c)$' | tee test_after.log
```

Result: build was already current; `backend_lir_to_bir_notes` passed, `00173`
passed, `00005` failed with store local-memory semantic rejection, and `00217`
failed with load local-memory semantic rejection. Proof log: `test_after.log`.
