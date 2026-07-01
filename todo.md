Status: Active
Source Idea Path: ideas/open/511_static_local_object_data_contract_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Static-Local Storage Identity Publication

# Current Packet

## Just Finished

Step 2 of `plan.md` traced static-local storage identity from frontend
publication into prepared selected object-data publication.

The static-local path begins in `Lowerer::lower_static_local_global` in
`src/frontend/hir/hir_types.cpp`. It creates a `GlobalVar` named
`__static_local_<function>_<id>`, records the declaration in the function
static-local maps, lowers the initializer through `lower_global_init`, indexes
the global, and appends it to `module_->globals`. Unlike ordinary
`Lowerer::lower_global`, it does not set `name_text_id`, `link_name_id`, or
namespace metadata. That means the later HIR-to-LIR global path forwards a
stable rendered name but an invalid `LinkNameId`.

The LIR and BIR paths preserve that missing identity rather than dropping it.
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp` copies `GlobalVar::link_name_id`
into `LirGlobal::link_name_id`, and `src/backend/bir/lir_to_bir/globals.cpp`
copies `LirGlobal::link_name_id` into `bir::Global::link_name_id`. Access
selection can still find the static global by `GlobalId` and raw rendered
name, which is why Step 1 saw three prepared direct global accesses for
`__static_local_eq_0`, but those access facts are not storage-object
authority. The selected object-data publisher in
`src/backend/prealloc/object_data.cpp` keys object identity on
`bir::Global::link_name_id`; with `kInvalidLinkName`, the current route emits
or observes no object label and RV64 reports `missing_object_label`.

The minimal Step 3 boundary is producer-side identity publication before
`populate_prepared_object_data_plans`: assign function-scope static storage a
real module link-name identity when the HIR static global is created, using
the same generated storage spelling that already drives raw emitted names.
After that, the existing HIR-to-LIR, LIR-to-BIR, and prealloc object-data
pipeline can publish `object_label`, label text, size, alignment,
initialized bytes, zero-fill ranges, and publication identity from prepared
producer facts. RV64 should continue to consume only
`PreparedGlobalObjectData`; it must not infer missing static-local authority
from symbol spelling or prepared direct-global access records.

## Suggested Next

Execute Step 3: publish `link_name_id` and stable link-name text for
function-scope static globals in `Lowerer::lower_static_local_global`, then add
focused coverage proving static-local identity, extent, and alignment reach
prepared selected object-data records without changing RV64 inference rules.

## Watchouts

- Keep the first implementation boundary at the producer identity source:
  `Lowerer::lower_static_local_global` should own the static-local
  `link_name_id`, and `populate_prepared_object_data_plans` should remain the
  selected object-data publication entry point.
- Direct global access facts for `__static_local_eq_0` are consumer evidence
  only. They prove the storage object is accessed at offset `0`, size `4`, and
  align `4`, but they must not become the source of object label, byte range,
  zero-fill, initializer, or relocation authority.
- Preserve existing fail-closed variants: missing storage identity must still
  diagnose `missing_object_label`; zero or unknown extent/alignment must still
  diagnose missing publication identity or byte range; unsupported
  initializer semantics must remain
  `invalid_pre_prepared_initializer_semantics`; missing initialized bytes,
  missing zero-fill, missing relocation, conflicting labels, conflicting
  bytes, conflicting zero-fill, conflicting byte ranges, and conflicting
  relocation facts must keep their verifier statuses.
- Externs, TLS/thread-local globals, GOT-required globals, marker-only
  unsupported data, dynamic initialization, ambiguous symbol-pointer
  relocation facts, and unrelated aggregate/ABI/F128 work remain outside this
  route unless a later plan explicitly owns them.

## Proof

- Trace-only packet; no build was required and `test_after.log` was not
  updated.
- Inspected the active plan/todo state, Step 1 probe artifacts under
  `build/agent_state/511_step1_static_local_object_data_evidence/930513-2/`,
  static-local lowering, HIR-to-LIR global forwarding, LIR-to-BIR global
  forwarding, prepared object-data publication, selected object-data verifier
  statuses, RV64 selected object-data admission, and existing focused tests.
