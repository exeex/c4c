Status: Active
Source Idea Path: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Local-Address Array-Element Authority Evidence

# Current Packet

## Just Finished

Completed Step 1 for idea 484 by auditing local-address/array-element
authority evidence from `pr22098-1.c`, `pr38048-1.c`, and `multi-ix.c`.

Selected Step 2 contract driver:

| Case | Accepted shape | Required authority |
| --- | --- | --- |
| `src/pr38048-1.c` | `decl a: int*[1] = mat#L0`; `det#L2 += a#L1[i#L3][0]` | local object `mat#L0: int[2][1]`, array decay/local-address derivation into `a#L1`, dynamic outer index `i#L3` with range `i < 2`, constant inner index `0`, scalar `int` element layout/range, and pointer-to-local-element provenance. |

Secondary/blocked evidence:

| Case | Shape | Disposition |
| --- | --- | --- |
| `src/pr22098-1.c` | `p#L1 = &<clit>#L3[(++a#L0)]`; `(*p#L1) != 1` | Accept only as secondary local element-address evidence once Step 2 requires explicit index/range and pointer-to-local-element provenance. |
| `src/pr22098-1.c` | `b#L2 = (ulong)p#L1`; `(*((int*)b#L2)) != 1` | Reject integer-pointer round-trip provenance from the first packet. |
| `src/multi-ix.c` | `i0#L41 = a0#L1[0]` through `i39#L80 = a39#L40[0]` | Reject as first driver because the row's latest failure is variadic `s`, and the case has variadic/va_arg/memset contamination. |

Required facts for Step 2:

- source local object identity and type/rank;
- array decay or local-address derivation from local object to pointer/view
  value;
- dynamic index value identity and explicit range proof;
- constant index identity where present;
- element type, size, alignment, offset, and range within the source object;
- pointer-to-local-element provenance tying the load address to the same local
  object and element path;
- unavailable/mismatch statuses for missing object, missing range,
  unsupported integer round-trip, aggregate/member contamination, variadic
  contamination, runtime/global/bootstrap/F128 boundaries, and raw inference.

Artifact:

- `build/agent_state/484_step1_local_address_array_element_authority/audit.md`

## Suggested Next

Execute Step 2: define the bounded local-address/array-element provenance
contract around the `pr38048-1.c` `a#L1[i#L3][0]` shape. Keep this as
producer-authority metadata only; do not consume scalar loads or implement RV64
lowering.

## Watchouts

- Do not accept integer-pointer round-trip provenance, including the
  `pr22098-1.c` `uintptr_t` path.
- Do not use `multi-ix.c` as the first driver; it is useful boundary evidence
  but contaminated by variadic/va_arg/memset paths.
- Do not infer local-address authority from testcase names, value names, raw
  dump order, final homes, or RV64 target fallback behavior.
- Keep global, aggregate/member, union/object-representation, variadic/va_arg,
  runtime/call, F128, complex, vector, volatile/atomic, bootstrap, and raw
  inference shapes fail-closed.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
