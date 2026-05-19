# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Local-Memory GEP Admission

# Current Packet

## Just Finished

Step 2 classification-only packet analyzed the remaining GEP-family admissions
after direct dynamic scalar local-array GEP repair. `test_before.log` still
records `00176`, `00181`, `00182`, `00195`, `00205`, and `00209` as
`gep local-memory semantic family`; the prior moved cases `00143`, `00157`,
and `00185` are not part of the remaining GEP admission owner.

Narrow `--codegen llvm` and `--dump-bir` diagnostics classify the residuals:

- Global dynamic scalar-array GEPs: `00176` (`swap`, `getelementptr i32, ptr
  @array, i64 %idx`) and `00181` (`PrintAll`, `getelementptr i32, ptr @A/@B/@C,
  i64 %idx`). These are globals whose LLVM spelling decays directly to scalar
  element pointers, not local stack arrays.
- Pointer-parameter/pointer-value GEPs: `00182` (`print_led`, repeated
  `getelementptr i8, ptr %buf, i64 1/3` after loading the pointer parameter
  slot) and `00209` (`f4`, `getelementptr ptr, ptr %fp, i64 %i` then indirect
  call). These need pointer-value array/provenance admission, not aggregate
  local slot indexing.
- Global dynamic aggregate member GEPs: `00195` (`point_array[my_point].x/y`)
  and `00205` (`cases[j].c[i]`, plus sibling `b/e/k` fields). These require
  dynamic global aggregate projection through struct fields and nested array
  fields.

Representative BIR dumps for `00176`, `00181`, and `00209` still stop before
handoff with the same `gep local-memory semantic family` note, confirming this
packet did not move or repair behavior.

## Suggested Next

Keep Step 2 for one bounded implementation packet: admit global dynamic
scalar-array GEPs for scalar element globals represented as `getelementptr T,
ptr @global, i64 %idx`, with `00176` as the primary representative and `00181`
as the same-shape proof partner. The likely owner is the global pointer-slot
branch in `lower_memory_gep_inst`, specifically the fallback around
`resolve_global_dynamic_pointer_array_access`, `DynamicGlobalScalarArrayAccess`,
and global type/array length lookup; it should publish semantic dynamic global
scalar-array access instead of rejecting scalar element arrays.

Do not combine that with the pointer-parameter cases (`00182`, `00209`) or the
global aggregate member cases (`00195`, `00205`) in the same packet.

## Watchouts

- Direct dynamic scalar local-array admission is no longer the blocker for
  representatives `00143`, `00157`, and `00185`; do not conflate their new
  machine/runtime failures with the old GEP admission failure.
- Focused GEP local-memory cases still failing in GEP family: `00176`,
  `00181`, `00182`, `00195`, `00205`, `00209`; classify them by semantic shape,
  not as one local-array bucket.
- `00176`/`00181` are the recommended next target because they share a plain
  global scalar-array dynamic index and avoid pointer-parameter provenance and
  nested aggregate projection.
- `00182` and `00209` likely belong to pointer-value GEP admission over pointer
  parameters or pointer-typed local slots; keep them separate from global array
  work.
- `00195` and `00205` likely belong to dynamic global aggregate projection,
  including scalar struct fields and nested member arrays; keep them separate
  from scalar global arrays until the simpler global scalar-array lane is
  proven.
- Store/load boundary checks: `00046`, `00140`, `00216`, `00218`.
- `00204` is a separate bootstrap global aggregate/array semantics gate; do
  not fold it into this owner without evidence.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, runner behavior, `test_after.log`, or
  `test_before.log`.
- Do not claim progress through filename matching or named-case shortcuts.
- Step 2 should not special-case `00143`; the rejected form is semantic:
  dynamic GEP into scalar local arrays whose elements are represented as local
  slots.
- Store/load boundary notes for Step 3: `00046` needs nested aggregate
  subobject scalar stores through local address provenance; `00216` needs
  aggregate load/copy from pointer-derived local memory.

## Proof

Classification-only packet; per delegation, did not rerun broad backend regex
and did not modify `test_before.log` or `test_after.log`.

Diagnostics run:

- Read current `test_before.log` focused baseline for `00176`, `00181`,
  `00182`, `00195`, `00205`, and `00209`.
- Ran `./build/c4cll --dump-bir --target aarch64-linux-gnu` on representatives
  `00176`, `00181`, and `00209`; each stopped with the current GEP family
  admission failure.
- Ran `./build/c4cll --codegen llvm --target aarch64-linux-gnu` to `/tmp` for
  `00176`, `00181`, `00182`, `00195`, `00205`, and `00209` and classified the
  LLVM GEP shapes above.
