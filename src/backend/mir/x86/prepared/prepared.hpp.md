# `prepared/prepared.hpp` contract

## Legacy Evidence

The legacy x86 tree mostly reached into prepared backend data through broader
codegen helpers instead of a bounded query seam. This companion exists to keep
the new public surface honest about which prepared facts x86 may ask for
directly.

## Design Contract

This header is the single public header for `x86/prepared/`.

Owned inputs:

- `PreparedBirModule`
- resolved or unresolved function-name focus for bounded queries
- already-published BIR immediates when rendering narrow operand text

Owned outputs:

- `prepared::Query` as the only public prepared-data lookup context for x86
- `prepared::FastPath` summaries for early route classification
- `prepared::Operand` render records for immediate-only operand text

Invariants:

- this seam may read prepared module facts but must not publish new prealloc
  answers
- `Query` answers are bounded to the selected prepared function and may return
  null when the function is absent
- fast-path classification is advisory for x86 routing; it must not become a
  second preparation pipeline
- operand rendering here stays limited to immediate formatting, not general
  addressing or stack-slot reconstruction

Direct dependencies:

- `backend/prealloc/prealloc.hpp` for canonical prepared module, addressing,
  and value-location publications
- `backend/bir/bir.hpp` for immediate value kinds consumed by
  `render_immediate_operand(...)`

Deferred behavior or upstream gaps:

- frame layout, dynamic-stack motion, and register-bank authority remain owned
  by `prealloc/`
- whole-function addressing text and full instruction selection stay with
  `module/` and `lowering/`

Must not own:

- module emission
- semantic lowering policy
- hidden fallback orchestration
- prepared-data archaeology beyond the published query helpers
