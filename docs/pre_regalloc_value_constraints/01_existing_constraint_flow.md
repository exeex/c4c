# Existing `PreparedAllocationConstraint` Flow

## Scope and result

This trace follows named BIR value identity through liveness, constraint
publication, register assignment, and observable validation. The central fact
is that `PreparedAllocationConstraint` is currently a published description,
not an input consumed by the allocator. There is one production site, no
production read of `PreparedRegallocFunction::constraints`, and test-only reads
that validate the published fields.

The earliest missing boundary for a general named-value fixed/preferred
register request is before `BirPreAlloc::run_regalloc`: neither BIR nor
`PreparedLivenessValue` carries such a request into the single constraint
construction site. Moreover, adding only a published field value there would
not affect allocation, because assignment independently rebuilds target pools
from `PreparedRegallocValue::crosses_call` and never reads the constraint row.
This is a flow finding, not a recommendation about which layer should own a
future carrier; that decision belongs to Step 2.

## End-to-end trace

### 1. BIR named values become stable liveness identities

- `src/backend/prealloc/liveness.cpp` builds a `DenseValueSet` from each BIR
  function in `BirPreAlloc::run_liveness` and assigns monotonically increasing
  `PreparedValueId` values to the dense entries.
- `build_liveness_value` copies `value_id`, interned `function_name` and
  `value_name`, BIR `type`, `PreparedValueKind`, `address_taken`, and
  `requires_home_slot` into `PreparedLivenessValue`. It derives `crosses_call`
  by checking whether a computed call point lies strictly inside the live
  interval.
- `src/backend/prealloc/liveness.hpp` shows the complete
  `PreparedLivenessValue` schema. It has identity, type/kind, home-slot facts,
  def/use points, interval, and `crosses_call`; it has no fixed, preferred, or
  forbidden physical-register request.

Thus the semantic/liveness input reaching regalloc can identify a named value
and describe its lifetime and storage eligibility, but cannot distinguish two
otherwise equivalent values by a requested physical register.

### 2. Register class and width are classified before constraint publication

- `src/backend/prealloc/regalloc/classification.cpp` implements
  `resolve_register_class` and `resolve_register_group_width`.
- Class normally follows BIR `TypeKind` through `classify_register_class`.
  Width normally follows `bir::vrm_register_group_width` or defaults to one.
- The only named-value override on this path is
  `PreparedRegisterGroupOverride`, found by interned function/value identity.
  `src/backend/prealloc/inline_asm.cpp` publishes those overrides from inline
  assembly register-group metadata, and `src/backend/prealloc/prealloc.cpp`
  calls `populate_inline_asm_register_group_overrides` between liveness and
  regalloc.
- This override selects register class and contiguous group width. It does not
  carry a physical register name, placement, fixed assignment, or preference.

### 3. `run_regalloc` is the sole constraint producer

`src/backend/prealloc/regalloc.cpp`, in `BirPreAlloc::run_regalloc`, is the only
production construction site for `PreparedAllocationConstraint`:

1. It copies `PreparedLivenessValue` identity into a parallel
   `PreparedRegallocValue` and computes register class/width.
2. It omits constraints for values with class `None` and stack-object values.
3. For eligible values it appends one constraint keyed by `value_id`.
4. `requires_register` and `requires_home_slot` come from liveness home-slot
   eligibility.
5. `cannot_cross_call` comes from `crosses_call` for non-float classes.
6. Preferred and forbidden names/placements are materialized from the target
   profile: call-crossing values publish callee-saved preference and
   caller-saved prohibition; other values publish caller-saved preference.
7. `fixed_register_name` and `fixed_register_placement` are unconditionally
   `std::nullopt`.

`src/backend/prealloc/regalloc.hpp` defines the row and stores the rows in
`PreparedRegallocFunction::constraints`. Repository-wide symbol search finds
no second production constructor.

### 4. Assignment does not consume the published row

The allocation loop in `BirPreAlloc::run_regalloc` operates on
`PreparedRegallocValue` and liveness facts:

- allocation order is interval start, then derived priority, then `value_id`;
- `assign_from_pool` selects values by `PreparedRegallocValue::crosses_call`;
- its pool callbacks call `callee_saved_register_spans` or
  `caller_saved_register_spans` directly using target profile, class, and group
  width;
- `regalloc_detail::choose_register_span` in
  `src/backend/prealloc/regalloc/assignment.cpp` returns the first candidate
  that does not overlap active assignments;
- later fallback assigns stack slots.

No allocator branch looks up `regalloc_function.constraints`, and no call to
`choose_register_span` receives a `PreparedAllocationConstraint`. The
preferred/forbidden fields currently describe the same policy that assignment
independently derives; they do not govern it. The fixed fields have neither a
producer value nor a consumer.

### 5. Current consumers are observational

Repository-wide search for `.constraints` and the constraint field names finds
only these consumers:

- `tests/backend/bir/backend_prepare_liveness_test.cpp::find_constraint`
  searches published rows by `value_id`.
- The same test validates general-register/home-slot classification for phi and
  byval values and validates caller/callee saved names and structured placements
  for call-crossing and local values.

There is no prepared-printer output for `PreparedAllocationConstraint`, no
target consumer, and no validation routine that checks a fixed/preferred row
against an assigned register. ABI register selection exists separately through
helpers such as `call_arg_destination_register_name` and
`call_result_destination_register_name` in
`src/backend/prealloc/target_register_profile.cpp`; those helpers feed ABI move
and binding records, not `PreparedAllocationConstraint` construction or the
linear-scan candidate pool.

## Input classification

| Input family | Current supported input | Effect on published constraint | Effect on actual assignment | Classification |
|---|---|---|---|---|
| Semantic BIR identity | Named value, function, type, value kind | Supplies `value_id`; type contributes class/width | Identifies and orders the corresponding regalloc value | Supported semantic input |
| Semantic storage requirement | byval/sret/address-linked stack-object facts propagated as `requires_home_slot` | Sets `requires_register` / `requires_home_slot` | Directly forces stack-slot handling through `PreparedRegallocValue::requires_home_slot` | Supported semantic input |
| Liveness | interval, use points, call points, `crosses_call`, loop-weighted priority | Sets `cannot_cross_call`; selects published preferred/forbidden pool description | Directly selects caller/callee pool, order, eviction, and spill behavior | Derived analysis/policy |
| Inline-asm group metadata | Function/value keyed class and contiguous-width override | Changes class and width fields | Changes target candidate-span class/width | Supported prepared semantic override, limited to class/width |
| Target profile | Architecture caller/callee saved names and structured spans | Populates preferred/forbidden names and placements | Independently supplies candidate pools | Derived target policy |
| ABI argument/result metadata | ABI class, register passing, argument/result index | No `PreparedAllocationConstraint` input | Used by separate ABI move/binding helpers, not general value assignment | Supported ABI input on a separate route |
| General named-value fixed register | None | Fixed name/placement always absent | No consumer path | Missing input and missing enforcement |
| General named-value preferred/forbidden registers | None beyond call-crossing target policy | Lists are derived from caller/callee policy only | Published lists are not consumed | Missing semantic input; current rows are descriptive policy |

## Earliest missing general-value boundary

The first absent fact is a producer-owned request associated with the stable
function/value identity before `BirPreAlloc::run_regalloc` constructs
`PreparedAllocationConstraint`. `PreparedLivenessValue` is the immediate input
to that construction and exposes no such field; current BIR named-value
definitions likewise expose no general fixed/preferred allocation request on
this flow. Therefore `run_regalloc` can only write `nullopt` fixed fields and
target-policy-derived preference lists.

There is a second, downstream discontinuity at the same regalloc phase:
allocation never consumes the row it publishes. Any later design must account
for both facts—semantic request ingress and allocator enforcement. Merely
setting `fixed_register_name`, rewriting the joined-branch fixture, or changing
the published preference list would leave allocation behavior unchanged and
would not establish coherent producer authority.

## Evidence checks used for this trace

- AST `type-refs` on `src/backend/prealloc/regalloc.cpp` located the sole
  `PreparedAllocationConstraint` construction in `BirPreAlloc::run_regalloc`.
- AST definition lookup located `BirPreAlloc::run_liveness` in
  `src/backend/prealloc/liveness.cpp`.
- Repository-wide searches covered `PreparedAllocationConstraint`, all six
  fixed/preferred/forbidden fields, `.constraints`, register-group overrides,
  and candidate-pool helpers.
- Direct source inspection was limited to the definitions and call sites named
  above after AST/search localization.
