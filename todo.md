Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting one exact generic family consumer:
`LirAbsOp.int_type` verifier/printer.

Selected evidence:
- `src/codegen/lir/verify.cpp`: `verify_inst` currently calls
  `require_module_type_ref(mod, op->int_type, "LirAbsOp.int_type")` for
  `LirAbsOp`; `verify_abs_op_authority` already requires integer semantic
  authority when native result authority is present.
- `src/codegen/lir/lir_printer.cpp`: the `LirAbs` printer currently calls
  `require_type_ref(op->int_type, "LirAbsOp.int_type")` and emits
  `@llvm.abs.<type>` generically.
- Nearby focused same-feature coverage already exists around scalar abs
  authority in `tests/frontend/frontend_lir_call_type_ref_test.cpp`, plus
  backend BIR abs receipt/rejection tests.

Required overload: a scalar integer/integer type-family type-ref requirement
for `LirAbsOp.int_type`, using native semantic family authority and ownership
checks rather than display-text parsing or rendered-printer output.

Started the Step 2/Step 3 implementation packet locally after a stalled
executor delegation. The code scope remains limited to the selected
`LirAbsOp.int_type` verifier/printer consumer.

Completed the selected `LirAbsOp.int_type` packet:

- Added `render_integer_type_ref`, a bounded integer-family helper that checks
  native `LirTypeRef` kind/width authority and renders from structured width
  instead of reclassifying display text.
- Migrated only the `LirAbsOp.int_type` verifier and printer callsites to the
  bounded helper.
- Added focused stale-display coverage proving scalar abs verification and
  printing use native integer width authority, not mutable display text.

Completed Step 5 inventory for the next 846 packet. 846 still owns generic
family consumers, so the next selected surface is `LirSelectOp.type_str`
verifier/printer:

- `src/codegen/lir/verify.cpp` still verifies the selected field with
  `require_module_type_ref(mod, op->type_str, "LirSelectOp.type_str")`.
- `src/codegen/lir/lir_printer.cpp` still renders the selected field with
  `require_type_ref(op->type_str, "LirSelectOp.type_str")`.
- `verify_select_op_authority` already treats `op.type_str.kind() ==
  LirTypeKind::Integer` as the scalar integer authority claim and rejects a
  native scalar select result when the selected type is not integer.
- Focused scalar select coverage exists in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`, with backend BIR
  receipt/rejection coverage in `backend_lir_to_bir_interface`.

Completed the selected `LirSelectOp.type_str` packet:

- Reused `render_integer_type_ref` for the select verifier/printer type field.
- Migrated only the `LirSelectOp.type_str` verifier and printer callsites.
- Added focused stale-display coverage proving scalar select printing uses
  native integer width authority rather than mutable display text.

Completed Step 5 inventory for the next 846 packet. 846 still owns generic
family consumers, so the next selected surface is the integer compare branch of
`LirCmpOp.type_str` verifier/printer:

- `src/codegen/lir/verify.cpp` still verifies `LirCmpOp.type_str` with
  `require_module_type_ref(mod, op->type_str, "LirCmpOp.type_str")`.
- `src/codegen/lir/lir_printer.cpp` still renders `LirCmpOp.type_str` with
  `require_type_ref(op->type_str, "LirCmpOp.type_str")`.
- `verify_cmp_op_authority` already distinguishes integer and floating
  compares; the next packet must migrate only the non-floating integer branch
  and preserve the existing floating compare behavior.
- Focused integer compare coverage exists in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`, with backend BIR
  comparison receipt/rejection coverage in `backend_lir_to_bir_interface`.

Completed the selected integer `LirCmpOp.type_str` packet:

- Reused `render_integer_type_ref` for only non-floating compare verifier and
  printer type fields.
- Preserved floating compare type refs on the existing generic path.
- Added focused stale-display coverage proving integer compare printing uses
  native integer width authority rather than mutable display text.

Completed Step 5 inventory for the next 846 packet. 846 still owns a generic
integer scalar renderer inside the already selected `LirBinOp`
`compact_scalar_type` carrier:

- `compact_scalar_binop_type` already validates the carrier with
  `LirCompactScalarType::from_type_ref` and requires it to mirror
  `LirBinOp.type_str`.
- `src/codegen/lir/lir_printer.cpp` still renders the selected compact scalar
  carrier with generic `require_type_ref(*type, "LirBinOp.compact_scalar_type",
  true)`.
- The next packet must migrate only the integer compact-scalar rendering branch
  to native width authority and preserve floating compact scalar rendering.
- Focused compact-scalar wrong-family/stale-mirror coverage already exists in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`.

Completed the selected integer `LirBinOp.compact_scalar_type` packet:

- Reused `render_integer_type_ref` for integer compact-scalar verifier and
  printer branches.
- Preserved floating compact scalar rendering and unselected `LirBinOp`
  generic type refs.
- Added focused stale-display coverage proving integer compact-scalar binop
  rendering uses native integer width authority rather than mutable display
  text.

Completed Step 5 inventory for the next 846 packet. 846 still owns the
floating compare branch of `LirCmpOp.type_str` verifier/printer:

- The previous integer compare packet left `op->is_float` on the existing
  generic type-ref path.
- `verify_cmp_op_authority` already requires floating type authority when
  `op.is_float` is true.
- `LirTypeRef` retains `builtin_type()` for floating builtins independently of
  mutable display text.
- Focused floating compare coverage exists in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`.

Completed the selected floating `LirCmpOp.type_str` packet:

- Added `render_floating_type_ref`, a bounded floating builtin renderer that
  uses native `builtin_type()` authority.
- Migrated only the floating compare verifier/printer path.
- Added focused stale-display coverage proving floating compare printing uses
  native floating builtin authority rather than mutable display text.

Completed Step 5 inventory for the next 846 packet. 846 still owns floating
compact-scalar `LirBinOp.compact_scalar_type` rendering:

- The integer compact-scalar binop packet left floating compact scalar carriers
  on the generic `require_type_ref(*type, "LirBinOp.compact_scalar_type", true)`
  path.
- `compact_scalar_binop_type` already validates floating compact scalar
  carriers with `LirCompactScalarType::from_type_ref`.
- `render_floating_type_ref` now exists for native floating builtin rendering.
- Focused compact-scalar integer/floating coverage exists in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`.

Completed the selected floating `LirBinOp.compact_scalar_type` packet:

- Reused `render_floating_type_ref` for the floating compact-scalar printer
  branch.
- Preserved unselected `LirBinOp.type_str` and non-scalar fallback rendering.
- Added focused stale-display coverage proving floating compact-scalar binop
  rendering uses native floating builtin authority rather than mutable display
  text.

Completed the hook-mandated code and baseline review after the floating binop
packet. The attempted full-suite baseline regressed from `3038/3038` passing to
`2881/3038` passing and was rejected. Reproduction showed the compare helper
had been applied too broadly to non-floating pointer compares; the verifier and
printer are now narrowed so only integer `LirCmpOp.type_str` uses
`render_integer_type_ref`, while pointer and other non-integer `icmp` types
stay on the generic path.

Completed Step 5 inventory for the next 846 packet. 846 still owns integer
native-return `LirRet.type_str` verifier/printer rendering:

- `src/codegen/lir/verify.cpp` still verified `LirRet.type_str` with
  `require_type_ref(ret->type_str, "LirRet.type_str", true)`.
- `src/codegen/lir/lir_printer.cpp` still rendered valued returns with
  `require_type_ref(ret->type_str, "LirRet.type_str")`.
- `verify_return_value_parameter_authority` and authoritative scalar return
  checks already require integer type authority for native scalar returns.
- `StmtEmitter::emit_term_ret` already publishes native integer
  `LirReturnValueParameterAuthority` when a direct-scalar parameter is
  returned.

Completed the selected integer native-return `LirRet.type_str` packet:

- Reused `render_integer_type_ref` for authoritative integer return verifier
  and printer branches.
- Preserved void returns and raw compatibility returns on the existing generic
  paths.
- Added focused stale-display coverage proving integer native return rendering
  uses native integer width authority rather than mutable display text.

Started the Step 5 selected native integer scalar load packet. 846 still owns
integer native-load `LirLoadOp.type_str` verifier/printer rendering:

- `src/codegen/lir/verify.cpp` still verifies `LirLoadOp.type_str` with
  `require_module_type_ref(mod, op->type_str, "LirLoadOp.type_str", true)`.
- `src/codegen/lir/lir_printer.cpp` still renders loads with
  `require_type_ref(op->type_str, "LirLoadOp.type_str", true)`.
- `LirLoadOp.requires_native_result_authority`, `LirLoadOp.result`, and
  local/global load authority checks already publish the selected native
  result facts.
- The packet must migrate only selected native integer scalar loads and
  preserve raw compatibility, pointer, aggregate, store, cast, gep, phi, call,
  return, vector, va_arg, switch, and universal API behavior.

Completed the selected native integer scalar load `LirLoadOp.type_str` packet:

- Reused `render_integer_type_ref` for only authoritative integer load
  verifier and printer branches.
- Preserved raw compatibility loads and non-integer load types on the existing
  generic paths.
- Added focused stale-display coverage proving a selected native integer
  global load renders from native integer width authority rather than mutable
  display text.

Completed Step 5 inventory for the next 846 packet. 846 still owns selected
native integer scalar store `LirStoreOp.type_str` verifier/printer rendering:

- `src/codegen/lir/verify.cpp` still verifies `LirStoreOp.type_str` with
  `require_module_type_ref(mod, op->type_str, "LirStoreOp.type_str", true)`.
- `src/codegen/lir/lir_printer.cpp` still renders stores with
  `require_type_ref(op->type_str, "LirStoreOp.type_str", true)`.
- `LirStoreOp.requires_native_store_authority`, native store value authority,
  and local store authority checks already publish the selected native integer
  store facts.
- The packet must migrate only selected native integer scalar stores and
  preserve raw compatibility, global store compatibility, pointer, aggregate,
  load, cast, gep, phi, call, return, vector, va_arg, switch, and universal API
  behavior.

Completed the selected native integer scalar store `LirStoreOp.type_str`
packet:

- Reused `render_integer_type_ref` for only authoritative integer store
  verifier and printer branches.
- Preserved raw compatibility stores and non-integer store types on the
  existing generic paths.
- Added focused stale-display coverage proving a selected native integer local
  store renders from native integer width authority rather than mutable display
  text.

Started the Step 5 selected native integer local allocation packet. 846 still
owns selected native integer local allocation `LirAllocaOp.type_str`
verifier/printer rendering:

- `src/codegen/lir/verify.cpp` still verifies `LirAllocaOp.type_str` with
  `require_module_type_ref(mod, op->type_str, "LirAllocaOp.type_str")`.
- `src/codegen/lir/lir_printer.cpp` still renders allocas with
  `require_type_ref(op->type_str, "LirAllocaOp.type_str")`.
- `LirAllocaOp.local_object_authority` already ties the result pointer and
  pointee type facts to the selected local object allocation.
- The packet must migrate only selected native integer local allocas and
  preserve raw compatibility allocas, non-integer/pointer/aggregate allocas,
  load, store, cast, gep, phi, call, return, vector, va_arg, switch, and
  universal API behavior.

Completed the selected native integer local allocation `LirAllocaOp.type_str`
packet:

- Reused `render_integer_type_ref` for only authority-bearing integer local
  allocation verifier and printer branches.
- Preserved raw compatibility allocas and non-integer allocation types on the
  existing generic paths.
- Added focused stale-display coverage proving a selected native integer
  alloca renders from native integer width authority rather than mutable
  display text.

## Suggested Next

Execute `plan.md` Step 5: inventory remaining universal classifier, renderer,
mutable semantic string, and implicit conversion callers. Classify the next
exact 846-owned consumer or record the handoff/blocker decision if no 846
consumer is ready.

## Watchouts

Do not delete universal-model APIs yet; idea 847 owns terminal deletion after
846 gates are accepted. Do not parse display text, runtime text, diagnostics,
or printer output as semantic state. Do not absorb producer/store construction,
Raw-BIR receipt, 734 receiver repair, or 797 convergence.

Non-goals for the next packet: no broad verifier/printer rewrite, no generic
helper deletion unless no semantic callers remain, no dispatch migration beyond
one selected surface, no producer/carrier repair, and no expectation downgrade
or testcase-shaped special case. Do not migrate `LirCmpOp`, `LirBinOp`, vector,
aggregate, load/store, or cast type refs in the select packet.

For the compare packet, do not migrate floating compares, `LirBinOp`, vector,
aggregate, load/store, or cast type refs.

For the binop packet, do not migrate floating compact scalar rendering,
unselected `LirBinOp.type_str`, vector, aggregate, load/store, or cast type
refs.

For the floating compare packet, do not migrate floating `LirBinOp`, casts,
load/store, vector, aggregate, or non-builtin floating surfaces.

For the floating binop packet, do not migrate unselected `LirBinOp.type_str`,
casts, load/store, vector, aggregate, or non-builtin floating surfaces.

## Proof

Step 1 proof is source trace and lifecycle state only. Planning-file check:
`git diff --check`.

Completed Step 2 through Step 4 proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$' --output-on-failure`
- `git diff --check`

Step 5 trace selected `LirSelectOp.type_str` for the next bounded packet.
Completed select packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$' --output-on-failure`
- `git diff --check`

Next Step 5 decision is trace/lifecycle state unless it selects and records
another bounded implementation packet.

Post-review repair proof:

- `cmake --build build`
- `ctest --test-dir build -R '^(frontend_lir_call_type_ref|frontend_lir_global_label_address_initializer|frontend_lir_label_address_rvalue_probe)$' --output-on-failure`
- `ctest --test-dir build -R '^cpp_positive_sema_free_operator_eq_overload_frontend_cpp$' --output-on-failure`
- `git diff --check`

Step 5 trace selected the floating `LirBinOp.compact_scalar_type` rendering
branch for the next bounded packet. Completed floating binop compact packet
proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `git diff --check`

Next Step 5 decision is trace/lifecycle state unless it selects and records
another bounded implementation packet.

Step 5 trace selected the floating branch of `LirCmpOp.type_str` for the next
bounded packet. Completed floating compare packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `git diff --check`

Next Step 5 decision is trace/lifecycle state unless it selects and records
another bounded implementation packet.

Step 5 trace selected the integer `LirBinOp.compact_scalar_type` rendering
branch for the next bounded packet. Completed binop compact integer packet
proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `git diff --check`

Next Step 5 decision is trace/lifecycle state unless it selects and records
another bounded implementation packet.

Step 5 trace selected the integer branch of `LirCmpOp.type_str` for the next
bounded packet. Completed integer compare packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$' --output-on-failure`
- `git diff --check`

Step 5 trace selected the integer native-return `LirRet.type_str` verifier and
printer branch for the next bounded packet. Completed return packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `git diff --check`

Step 5 trace selected the native integer scalar `LirLoadOp.type_str` verifier
and printer branch for the next bounded packet. Completed load packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `git diff --check`

Proof log: `test_after.log`.

Step 5 trace selected the native integer scalar `LirStoreOp.type_str` verifier
and printer branch for the next bounded packet. Completed store packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `git diff --check`

Proof log: `test_after.log`.

Step 5 trace selected the native integer local allocation
`LirAllocaOp.type_str` verifier and printer branch for the next bounded
packet. Completed alloca packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
- `git diff --check`

Proof log: `test_after.log`.

Next Step 5 decision is trace/lifecycle state unless it selects and records
another bounded implementation packet.
