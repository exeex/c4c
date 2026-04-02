#include "extern_lowering.hpp"

#include "call_decode.hpp"

namespace c4c::backend {

std::optional<std::vector<std::string>> infer_extern_param_types(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirExternDecl& decl) {
  std::optional<std::vector<std::string>> inferred;
  const std::string callee = "@" + decl.name;

  for (const auto& function : module.functions) {
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
        if (call == nullptr || call->callee.str() != callee) {
          continue;
        }

        const auto parsed_call = parse_backend_typed_call(*call);
        if (!parsed_call.has_value()) {
          continue;
        }

        std::vector<std::string> param_types;
        param_types.reserve(parsed_call->param_types.size());
        for (const auto type : parsed_call->param_types) {
          param_types.push_back(std::string(type));
        }
        if (!inferred.has_value()) {
          inferred = std::move(param_types);
          continue;
        }
        if (*inferred != param_types) {
          throw LirAdapterError(
              LirAdapterErrorKind::Unsupported,
              "minimal backend LIR adapter does not support extern declarations with inconsistent typed call surfaces");
        }
      }
    }
  }

  return inferred;
}

BackendFunction lower_extern_decl(const c4c::codegen::lir::LirModule& module,
                                  const c4c::codegen::lir::LirExternDecl& decl) {
  BackendFunction out;
  out.is_declaration = true;
  out.signature.linkage = "declare";
  out.signature.linkage_kind = BackendFunctionLinkage::Declare;
  out.signature.return_type = decl.return_type_str;
  out.signature.name = decl.name;

  if (out.signature.return_type.empty() || out.signature.name.empty()) {
    throw LirAdapterError(LirAdapterErrorKind::Malformed,
                          "minimal backend LIR adapter could not adapt extern declaration");
  }

  const auto param_types = infer_extern_param_types(module, decl);
  if (param_types.has_value()) {
    out.signature.params.reserve(param_types->size());
    for (const auto& type_str : *param_types) {
      out.signature.params.push_back(BackendParam{type_str, {}});
    }
  }

  return out;
}

}  // namespace c4c::backend
