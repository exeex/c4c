# `abi/x86_target_abi.cpp`

Primary role: implement reusable x86 target and calling-convention helpers for
the rest of the subsystem.

Owned inputs:

- target profile requests and ABI classifications from api, module, and
  lowering clients
- stable register naming and machine-policy questions that are currently
  scattered through `mod.cpp`

Owned outputs:

- concrete register-formatting helpers
- caller/callee-saved policy helpers
- ABI-classification and stack-alignment utilities consumed by canonical
  lowering families

Allowed indirect queries:

- `core/x86_codegen_types.hpp` for shared data carriers
- no direct dependency on prepared-route dispatcher internals

Forbidden knowledge:

- symbol/data section emission
- route selection and prepared shape matching
- output-buffer mutation beyond returning formatting facts or small helper
  values

Role classification:

- `abi`

Dependency direction:

1. answer stable machine-policy questions
2. return reusable facts to module, core, and lowering callers
3. avoid becoming a second orchestration layer
