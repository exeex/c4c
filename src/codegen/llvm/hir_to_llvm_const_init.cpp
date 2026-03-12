#include "hir_emitter.hpp"

namespace tinyc2ll::codegen::llvm_backend {

class HirEmitter::ConstInitEmitter {
   public:
    explicit ConstInitEmitter(HirEmitter& emitter) : emitter_(emitter) {}

    std::string emit_const_init(const TypeSpec& ts, const GlobalInit& init) {
      if (ts.array_rank > 0) return emit_const_array(ts, init);
      if (ts.is_vector && ts.vector_lanes > 0) return emit_const_vector(ts, init);
      if (ts.base == TB_VA_LIST && ts.ptr_level == 0) return "zeroinitializer";
      if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0)
        return emit_const_struct(ts, init);
      if (const auto* s = std::get_if<InitScalar>(&init))
        return emitter_.emit_const_scalar_expr(s->expr, ts);
      if (const auto* list = std::get_if<InitList>(&init)) {
        if (!list->items.empty()) return emit_const_init(ts, child_init_of(list->items.front()));
      }
      return (llvm_ty(ts) == "ptr") ? "null" : "zeroinitializer";
    }

    std::vector<std::string> emit_const_struct_fields(const TypeSpec& ts,
                                                      const HirStructDef& sd,
                                                      const GlobalInit& init,
                                                      std::vector<TypeSpec>* out_field_types = nullptr) {
      return emit_const_struct_fields_impl(ts, sd, init, out_field_types);
    }

   private:
    HirEmitter& emitter_;

    const Module& mod() const { return emitter_.mod_; }

    GlobalInit child_init_of(const InitListItem& item) const {
      return std::visit([&](const auto& v) -> GlobalInit {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
        else return GlobalInit(*v);
      }, item.value);
    }

    std::string emit_const_from_flat(const TypeSpec& ts, const InitList& list, size_t& cursor) {
      if (cursor >= list.items.size()) return emit_const_init(ts, GlobalInit(std::monostate{}));

      const auto& item = list.items[cursor];
      if (std::holds_alternative<std::shared_ptr<InitList>>(item.value)) {
        auto sub = std::get<std::shared_ptr<InitList>>(item.value);
        ++cursor;
        return emit_const_init(ts, GlobalInit(*sub));
      }

      const bool is_agg = is_vector_value(ts) || (ts.array_rank > 0) ||
          ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0);
      if (!is_agg) {
        const auto* scalar = std::get_if<InitScalar>(&item.value);
        ++cursor;
        if (scalar) return emitter_.emit_const_scalar_expr(scalar->expr, ts);
        return emit_const_init(ts, GlobalInit(std::monostate{}));
      }

      if (is_vector_value(ts)) {
        TypeSpec elem_ts = ts;
        elem_ts.is_vector = false;
        elem_ts.vector_lanes = 0;
        elem_ts.vector_bytes = 0;
        std::string out = "<";
        for (long long i = 0; i < ts.vector_lanes; ++i) {
          if (i) out += ", ";
          out += llvm_ty(elem_ts) + " ";
          if (cursor < list.items.size()) out += emit_const_from_flat(elem_ts, list, cursor);
          else out += emit_const_init(elem_ts, GlobalInit(std::monostate{}));
        }
        out += ">";
        return out;
      }

      if (ts.array_rank > 0) {
        TypeSpec elem_ts = drop_one_array_dim(ts);
        if (is_char_like(elem_ts.base) && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0) {
          const auto* scalar = std::get_if<InitScalar>(&item.value);
          if (scalar) {
            const Expr& e = emitter_.get_expr(scalar->expr);
            if (std::holds_alternative<StringLiteral>(e.payload)) {
              ++cursor;
              return emit_const_init(ts, GlobalInit(*scalar));
            }
          }
        }

        const long long n = ts.array_size;
        if (n <= 0) return "zeroinitializer";
        std::string out = "[";
        const std::string elem_ty = llvm_alloca_ty(elem_ts);
        for (long long i = 0; i < n; ++i) {
          if (i) out += ", ";
          out += elem_ty + " ";
          if (cursor < list.items.size()) out += emit_const_from_flat(elem_ts, list, cursor);
          else out += emit_const_init(elem_ts, GlobalInit(std::monostate{}));
        }
        out += "]";
        return out;
      }

      if (!ts.tag || !ts.tag[0]) {
        ++cursor;
        return "zeroinitializer";
      }
      const auto it = mod().struct_defs.find(ts.tag);
      if (it == mod().struct_defs.end()) {
        ++cursor;
        return "zeroinitializer";
      }
      const HirStructDef& sd = it->second;
      if (sd.is_union) {
        if (sd.fields.empty()) {
          ++cursor;
          return "zeroinitializer";
        }
        const int sz = sd.size_bytes;
        const auto* scalar = std::get_if<InitScalar>(&item.value);
        ++cursor;
        if (scalar) {
          const Expr& se = emitter_.get_expr(scalar->expr);
          long long val = 0;
          if (const auto* il = std::get_if<IntLiteral>(&se.payload)) val = il->value;
          else if (const auto* cl = std::get_if<CharLiteral>(&se.payload)) val = cl->value;
          if (val != 0) {
            std::string bytes(static_cast<size_t>(sz), '\0');
            for (int i = 0; i < sz; ++i)
              bytes[static_cast<size_t>(i)] = static_cast<char>((val >> (8 * i)) & 0xFF);
            return "{ [" + std::to_string(sz) + " x i8] c\"" + escape_llvm_c_bytes(bytes) + "\" }";
          }
        }
        return "{ [" + std::to_string(sz) + " x i8] zeroinitializer }";
      }

      std::vector<std::string> field_vals;
      field_vals.reserve(sd.fields.size());
      for (const auto& f : sd.fields) {
        field_vals.push_back(emit_const_init(emitter_.field_decl_type(f), GlobalInit(std::monostate{})));
      }
      for (size_t fi = 0; fi < sd.fields.size() && cursor < list.items.size(); ++fi) {
        field_vals[fi] = emit_const_from_flat(emitter_.field_decl_type(sd.fields[fi]), list, cursor);
      }
      return format_struct_literal(sd, field_vals);
    }

    std::string emit_const_vector(const TypeSpec& ts, const GlobalInit& init) {
      TypeSpec elem_ts = ts;
      elem_ts.is_vector = false;
      elem_ts.vector_lanes = 0;
      elem_ts.vector_bytes = 0;
      const long long n = ts.vector_lanes > 0 ? ts.vector_lanes : 1;
      std::vector<std::string> elems(static_cast<size_t>(n),
                                     emit_const_init(elem_ts, GlobalInit(std::monostate{})));
      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        if (n > 0) elems[0] = emitter_.emit_const_scalar_expr(scalar->expr, elem_ts);
      } else if (const auto* list = std::get_if<InitList>(&init)) {
        for (size_t i = 0; i < list->items.size() && i < elems.size(); ++i) {
          elems[i] = emit_const_init(elem_ts, child_init_of(list->items[i]));
        }
      }
      std::string out = "<";
      for (size_t i = 0; i < elems.size(); ++i) {
        if (i) out += ", ";
        out += llvm_ty(elem_ts) + " " + elems[i];
      }
      out += ">";
      return out;
    }

    std::string emit_const_array(const TypeSpec& ts, const GlobalInit& init) {
      const long long n = ts.array_size;
      if (n <= 0) return "zeroinitializer";
      TypeSpec elem_ts = drop_one_array_dim(ts);

      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        const Expr& e = emitter_.get_expr(scalar->expr);
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload);
            sl && ts.array_rank == 1 && is_char_like(elem_ts.base) && elem_ts.ptr_level == 0) {
          std::string bytes = bytes_from_string_literal(*sl);
          if ((long long)bytes.size() > n) bytes.resize(static_cast<size_t>(n));
          if ((long long)bytes.size() < n) bytes.resize(static_cast<size_t>(n), '\0');
          return "c\"" + escape_llvm_c_bytes(bytes) + "\"";
        }
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload);
            sl && sl->is_wide && ts.array_rank == 1 && elem_ts.ptr_level == 0) {
          std::vector<long long> vals = decode_wide_string_values(sl->raw);
          if ((long long)vals.size() > n) vals.resize(static_cast<size_t>(n));
          while ((long long)vals.size() < n) vals.push_back(0);
          std::string out = "[";
          const std::string elem_ty = llvm_alloca_ty(elem_ts);
          for (size_t i = 0; i < vals.size(); ++i) {
            if (i) out += ", ";
            out += elem_ty + " " + std::to_string(vals[i]);
          }
          out += "]";
          return out;
        }
        std::vector<std::string> elems(static_cast<size_t>(n), "zeroinitializer");
        if (n > 0) elems[0] = emitter_.emit_const_scalar_expr(scalar->expr, elem_ts);
        return format_array_literal(elem_ts, elems);
      }

      std::vector<std::string> elems(static_cast<size_t>(n), "zeroinitializer");
      if (const auto* list = std::get_if<InitList>(&init)) {
        size_t next_idx = 0;
        for (const auto& item : list->items) {
          size_t idx = next_idx;
          if (item.index_designator && *item.index_designator >= 0) {
            idx = static_cast<size_t>(*item.index_designator);
          }
          if (idx >= static_cast<size_t>(n)) continue;
          elems[idx] = emit_const_init(elem_ts, child_init_of(item));
          next_idx = idx + 1;
        }
      }
      return format_array_literal(elem_ts, elems);
    }

    std::optional<std::string> try_emit_ptr_from_char_init(const GlobalInit& init) {
      auto make_ptr_to_bytes = [&](const std::string& bytes) -> std::string {
        const std::string gname = emitter_.intern_str(bytes);
        const size_t len = bytes.size() + 1;
        return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
               gname + ", i64 0, i64 0)";
      };

      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        const Expr& e = emitter_.get_expr(scalar->expr);
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
          return make_ptr_to_bytes(bytes_from_string_literal(*sl));
        }
        return std::nullopt;
      }

      const auto* list = std::get_if<InitList>(&init);
      if (!list) return std::nullopt;

      std::string bytes;
      size_t next_idx = 0;
      for (const auto& item : list->items) {
        size_t idx = next_idx;
        if (item.index_designator && *item.index_designator >= 0) {
          idx = static_cast<size_t>(*item.index_designator);
        }
        if (idx > bytes.size()) bytes.resize(idx, '\0');

        GlobalInit child_init = child_init_of(item);
        const auto* child_scalar = std::get_if<InitScalar>(&child_init);
        if (!child_scalar) return std::nullopt;
        const Expr& ce = emitter_.get_expr(child_scalar->expr);
        int ch = 0;
        if (const auto* c = std::get_if<CharLiteral>(&ce.payload)) ch = c->value;
        else if (const auto* i = std::get_if<IntLiteral>(&ce.payload)) ch = static_cast<int>(i->value);
        else return std::nullopt;

        if (idx == bytes.size()) bytes.push_back(static_cast<char>(ch & 0xFF));
        else bytes[idx] = static_cast<char>(ch & 0xFF);
        next_idx = idx + 1;
      }
      return make_ptr_to_bytes(bytes);
    }

    TypeSpec resolve_flexible_array_field_ts(const HirStructField& f, const GlobalInit& init) {
      TypeSpec ts = emitter_.field_decl_type(f);
      if (!f.is_flexible_array) return ts;
      long long deduced = deduce_array_size_from_init(init);
      if (deduced <= 0) return ts;
      ts.array_rank = 1;
      ts.array_size = deduced;
      ts.array_dims[0] = deduced;
      return ts;
    }

    std::vector<std::string> emit_const_struct_fields_impl(const TypeSpec&,
                                                           const HirStructDef& sd,
                                                           const GlobalInit& init,
                                                           std::vector<TypeSpec>* out_field_types) {
      std::vector<TypeSpec> field_types;
      field_types.reserve(sd.fields.size());
      for (const auto& f : sd.fields) field_types.push_back(emitter_.field_decl_type(f));

      std::vector<std::string> field_vals;
      field_vals.reserve(sd.fields.size());
      for (size_t i = 0; i < sd.fields.size(); ++i) {
        field_vals.push_back(emit_const_init(field_types[i], GlobalInit(std::monostate{})));
      }

      auto update_field_type = [&](size_t idx, const GlobalInit& child_init) -> const TypeSpec& {
        if (idx + 1 == sd.fields.size() && sd.fields[idx].is_flexible_array) {
          field_types[idx] = resolve_flexible_array_field_ts(sd.fields[idx], child_init);
        }
        return field_types[idx];
      };

      auto is_string_scalar = [&](const InitScalar& s) -> bool {
        const Expr& e = emitter_.get_expr(s.expr);
        return std::holds_alternative<StringLiteral>(e.payload);
      };
      auto is_direct_array_scalar = [&](const TypeSpec& fts, const InitListItem& item) -> bool {
        if (fts.array_rank <= 0) return false;
        TypeSpec ets = drop_one_array_dim(fts);
        if (!is_char_like(ets.base) || ets.ptr_level != 0) return false;
        if (const auto* s = std::get_if<InitScalar>(&item.value)) return is_string_scalar(*s);
        return false;
      };
      auto find_field_index = [&](const InitListItem& item, size_t next_idx) -> std::optional<size_t> {
        size_t idx = next_idx;
        if (item.field_designator) {
          const auto fit = std::find_if(
              sd.fields.begin(), sd.fields.end(),
              [&](const HirStructField& f) { return f.name == *item.field_designator; });
          if (fit == sd.fields.end()) return std::nullopt;
          idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
        } else if (item.index_designator && *item.index_designator >= 0) {
          idx = static_cast<size_t>(*item.index_designator);
        }
        if (idx >= sd.fields.size()) return std::nullopt;
        return idx;
      };

      if (const auto* list = std::get_if<InitList>(&init)) {
        const bool field_mapped_list = std::all_of(
            list->items.begin(), list->items.end(),
            [](const InitListItem& item) { return item.field_designator.has_value(); });
        if (field_mapped_list) {
          size_t next_idx = 0;
          for (const auto& item : list->items) {
            const auto maybe_idx = find_field_index(item, next_idx);
            if (!maybe_idx) continue;
            const size_t idx = *maybe_idx;
            GlobalInit child_init = child_init_of(item);
            const TypeSpec& field_ts = update_field_type(idx, child_init);
            if (llvm_field_ty(sd.fields[idx]) == "ptr") {
              if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
                field_vals[idx] = *ptr_init;
                next_idx = idx + 1;
                continue;
              }
            }
            field_vals[idx] = emit_const_init(field_ts, child_init);
            next_idx = idx + 1;
          }
          if (out_field_types) *out_field_types = std::move(field_types);
          return field_vals;
        }

        bool use_cursor = false;
        if (list->items.size() > sd.fields.size()) {
          bool has_designators = false;
          for (const auto& item : list->items) {
            if (item.field_designator || item.index_designator) {
              has_designators = true;
              break;
            }
          }
          if (!has_designators) use_cursor = true;
        }
        if (!use_cursor) {
          size_t fi = 0;
          for (size_t li = 0; li < list->items.size() && fi < sd.fields.size(); ++li, ++fi) {
            const auto& item = list->items[li];
            if (item.field_designator || item.index_designator) continue;
            const TypeSpec& fts = field_types[fi];
            const bool f_is_agg = (fts.array_rank > 0) ||
                ((fts.base == TB_STRUCT || fts.base == TB_UNION) && fts.ptr_level == 0);
            if (f_is_agg && std::holds_alternative<InitScalar>(item.value) &&
                !is_direct_array_scalar(fts, item)) {
              use_cursor = true;
              break;
            }
          }
        }

        if (use_cursor) {
          size_t cursor = 0;
          for (size_t fi = 0; fi < sd.fields.size() && cursor < list->items.size(); ++fi) {
            const auto& item = list->items[cursor];
            if (item.field_designator) {
              const auto fit = std::find_if(
                  sd.fields.begin(), sd.fields.end(),
                  [&](const HirStructField& f) { return f.name == *item.field_designator; });
              if (fit != sd.fields.end()) {
                fi = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
              }
            }
            if (fi >= sd.fields.size()) break;
            if (std::holds_alternative<std::shared_ptr<InitList>>(item.value)) {
              auto sub = std::get<std::shared_ptr<InitList>>(item.value);
              ++cursor;
              GlobalInit child_init(*sub);
              const TypeSpec& field_ts = update_field_type(fi, child_init);
              if (llvm_field_ty(sd.fields[fi]) == "ptr") {
                if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
                  field_vals[fi] = *ptr_init;
                  continue;
                }
              }
              field_vals[fi] = emit_const_init(field_ts, child_init);
              continue;
            }

            GlobalInit child_init = child_init_of(item);
            const TypeSpec& field_ts = update_field_type(fi, child_init);
            const bool f_is_agg = (field_ts.array_rank > 0) ||
                ((field_ts.base == TB_STRUCT || field_ts.base == TB_UNION) && field_ts.ptr_level == 0);
            if (f_is_agg && !is_direct_array_scalar(field_ts, item)) {
              field_vals[fi] = emit_const_from_flat(field_ts, *list, cursor);
              continue;
            }

            const auto* scalar = std::get_if<InitScalar>(&item.value);
            ++cursor;
            if (!scalar) continue;
            if (llvm_field_ty(sd.fields[fi]) == "ptr") {
              if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
                field_vals[fi] = *ptr_init;
                continue;
              }
            }
            field_vals[fi] = emit_const_init(field_ts, child_init);
          }
        } else {
          size_t next_idx = 0;
          for (const auto& item : list->items) {
            size_t idx = next_idx;
            if (item.field_designator) {
              const auto fit = std::find_if(
                  sd.fields.begin(), sd.fields.end(),
                  [&](const HirStructField& f) { return f.name == *item.field_designator; });
              if (fit == sd.fields.end()) continue;
              idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
            } else if (item.index_designator && *item.index_designator >= 0) {
              idx = static_cast<size_t>(*item.index_designator);
            }
            if (idx >= sd.fields.size()) continue;

            GlobalInit child_init = child_init_of(item);
            const TypeSpec& field_ts = update_field_type(idx, child_init);
            if (llvm_field_ty(sd.fields[idx]) == "ptr") {
              if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
                field_vals[idx] = *ptr_init;
                next_idx = idx + 1;
                continue;
              }
            }
            field_vals[idx] = emit_const_init(field_ts, child_init);
            next_idx = idx + 1;
          }
        }
      } else if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        if (!sd.fields.empty()) {
          GlobalInit child_init(*scalar);
          const TypeSpec& field_ts = update_field_type(0, child_init);
          field_vals[0] = emit_const_init(field_ts, child_init);
        }
      }

      if (out_field_types) *out_field_types = std::move(field_types);
      return field_vals;
    }

    std::string emit_const_struct(const TypeSpec& ts, const GlobalInit& init) {
      if (!ts.tag || !ts.tag[0]) return "zeroinitializer";
      const auto it = mod().struct_defs.find(ts.tag);
      if (it == mod().struct_defs.end()) return "zeroinitializer";
      const HirStructDef& sd = it->second;
      if (sd.is_union) return emit_const_union(ts, sd, init);
      return format_struct_literal(sd, emit_const_struct_fields_impl(ts, sd, init, nullptr));
    }

    std::string emit_const_union(const TypeSpec& ts, const HirStructDef& sd, const GlobalInit& init) {
      if (sd.fields.empty()) return "zeroinitializer";
      const int sz = sd.size_bytes;
      auto zero_union = [&]() -> std::string {
        if (sz == 0) return "zeroinitializer";
        return "{ [" + std::to_string(sz) + " x i8] zeroinitializer }";
      };

      std::function<bool(const TypeSpec&, const GlobalInit&, std::vector<unsigned char>&)> encode_bytes;
      encode_bytes = [&](const TypeSpec& cur_ts, const GlobalInit& cur_init,
                         std::vector<unsigned char>& out) -> bool {
        const int cur_sz = std::max(1, sizeof_ts(mod(), cur_ts));
        out.assign(static_cast<size_t>(cur_sz), 0);
        if (cur_ts.ptr_level > 0 || cur_ts.is_fn_ptr) return false;

        if (cur_ts.array_rank > 0) {
          const long long n = cur_ts.array_size;
          if (n <= 0) return true;
          TypeSpec elem_ts = drop_one_array_dim(cur_ts);
          const int elem_sz = std::max(1, sizeof_ts(mod(), elem_ts));
          if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
            const Expr& e = emitter_.get_expr(scalar->expr);
            if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
              if (elem_ts.ptr_level == 0 && is_char_like(elem_ts.base)) {
                const std::string bytes = bytes_from_string_literal(*sl);
                const size_t max_n = static_cast<size_t>(n);
                for (size_t i = 0; i < max_n && i < bytes.size(); ++i) {
                  out[i * static_cast<size_t>(elem_sz)] = static_cast<unsigned char>(bytes[i]);
                }
                if (bytes.size() < max_n) {
                  out[bytes.size() * static_cast<size_t>(elem_sz)] = 0;
                }
                return true;
              }
            }
            std::vector<unsigned char> elem;
            if (encode_bytes(elem_ts, cur_init, elem) && !elem.empty()) {
              const size_t copy_n = std::min<size_t>(static_cast<size_t>(elem_sz), elem.size());
              std::memcpy(out.data(), elem.data(), copy_n);
              return true;
            }
            return false;
          }
          if (const auto* list = std::get_if<InitList>(&cur_init)) {
            long long next_idx = 0;
            for (const auto& item : list->items) {
              long long idx = next_idx;
              if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
              if (idx >= 0 && idx < n) {
                std::vector<unsigned char> elem;
                if (encode_bytes(elem_ts, child_init_of(item), elem)) {
                  const size_t off = static_cast<size_t>(idx) * static_cast<size_t>(elem_sz);
                  const size_t copy_n = std::min<size_t>(static_cast<size_t>(elem_sz), elem.size());
                  std::memcpy(out.data() + off, elem.data(), copy_n);
                }
              }
              next_idx = idx + 1;
            }
            return true;
          }
          return true;
        }

        if ((cur_ts.base == TB_STRUCT || cur_ts.base == TB_UNION) && cur_ts.tag && cur_ts.tag[0]) {
          const auto sit = mod().struct_defs.find(cur_ts.tag);
          if (sit == mod().struct_defs.end()) return false;
          const HirStructDef& cur_sd = sit->second;
          if (cur_sd.is_union) {
            size_t fi = 0;
            GlobalInit child = GlobalInit(std::monostate{});
            bool has_child = false;
            if (const auto* list = std::get_if<InitList>(&cur_init)) {
              if (!list->items.empty()) {
                const auto& item0 = list->items.front();
                bool matched_union_field = false;
                if (item0.field_designator) {
                  const auto fit = std::find_if(
                      cur_sd.fields.begin(), cur_sd.fields.end(),
                      [&](const HirStructField& f) { return f.name == *item0.field_designator; });
                  if (fit != cur_sd.fields.end()) {
                    fi = static_cast<size_t>(std::distance(cur_sd.fields.begin(), fit));
                    child = child_init_of(item0);
                    has_child = true;
                    matched_union_field = true;
                  }
                } else if (item0.index_designator && *item0.index_designator >= 0) {
                  fi = static_cast<size_t>(*item0.index_designator);
                  child = child_init_of(item0);
                  has_child = true;
                  matched_union_field = true;
                }
                if (!matched_union_field) {
                  fi = 0;
                  child = (list->items.size() == 1 &&
                           std::holds_alternative<std::shared_ptr<InitList>>(item0.value))
                      ? GlobalInit(*std::get<std::shared_ptr<InitList>>(item0.value))
                      : GlobalInit(*list);
                  has_child = true;
                }
              }
            } else if (std::holds_alternative<InitScalar>(cur_init)) {
              child = cur_init;
              has_child = true;
            }
            if (!has_child || fi >= cur_sd.fields.size()) return true;
            std::vector<unsigned char> fb;
            if (!encode_bytes(emitter_.field_decl_type(cur_sd.fields[fi]), child, fb)) return false;
            const size_t copy_n = std::min(out.size(), fb.size());
            std::memcpy(out.data(), fb.data(), copy_n);
            return true;
          }

          if (const auto* list = std::get_if<InitList>(&cur_init)) {
            size_t next_field = 0;
            for (const auto& item : list->items) {
              size_t fi = next_field;
              if (item.field_designator) {
                const auto fit = std::find_if(
                    cur_sd.fields.begin(), cur_sd.fields.end(),
                    [&](const HirStructField& f) { return f.name == *item.field_designator; });
                if (fit == cur_sd.fields.end()) continue;
                fi = static_cast<size_t>(std::distance(cur_sd.fields.begin(), fit));
              } else if (item.index_designator && *item.index_designator >= 0) {
                fi = static_cast<size_t>(*item.index_designator);
              }
              if (fi >= cur_sd.fields.size()) continue;
              std::vector<unsigned char> fb;
              if (encode_bytes(emitter_.field_decl_type(cur_sd.fields[fi]), child_init_of(item), fb)) {
                const size_t off = static_cast<size_t>(cur_sd.fields[fi].offset_bytes);
                const size_t copy_n = std::min(out.size() - off, fb.size());
                std::memcpy(out.data() + off, fb.data(), copy_n);
              }
              next_field = fi + 1;
            }
            return true;
          }

          if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
            if (!cur_sd.fields.empty()) {
              std::vector<unsigned char> fb;
              if (encode_bytes(emitter_.field_decl_type(cur_sd.fields[0]), GlobalInit(*scalar), fb)) {
                const size_t copy_n = std::min(out.size(), fb.size());
                std::memcpy(out.data(), fb.data(), copy_n);
              }
            }
            return true;
          }
          return true;
        }

        if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
          const Expr& e = emitter_.get_expr(scalar->expr);
          unsigned long long v = 0;
          if (const auto* il = std::get_if<IntLiteral>(&e.payload)) v = static_cast<unsigned long long>(il->value);
          else if (const auto* cl = std::get_if<CharLiteral>(&e.payload)) v = static_cast<unsigned long long>(cl->value);
          else return false;
          for (int i = 0; i < cur_sz; ++i) {
            out[static_cast<size_t>(i)] = static_cast<unsigned char>((v >> (8 * i)) & 0xFF);
          }
          return true;
        }
        if (const auto* list = std::get_if<InitList>(&cur_init)) {
          if (!list->items.empty()) {
            std::vector<unsigned char> inner;
            if (encode_bytes(cur_ts, child_init_of(list->items.front()), inner)) {
              const size_t copy_n = std::min(out.size(), inner.size());
              std::memcpy(out.data(), inner.data(), copy_n);
              return true;
            }
            return false;
          }
        }
        return true;
      };

      auto emit_union_from_field =
          [&](size_t field_idx, const GlobalInit& child_init) -> std::optional<std::string> {
        if (field_idx >= sd.fields.size()) return std::nullopt;
        std::vector<unsigned char> bytes(static_cast<size_t>(sz), 0);
        std::vector<unsigned char> field_bytes;
        if (!encode_bytes(emitter_.field_decl_type(sd.fields[field_idx]), child_init, field_bytes)) {
          return std::nullopt;
        }
        if (field_bytes.empty()) return std::nullopt;
        const size_t copy_n = std::min(bytes.size(), field_bytes.size());
        std::memcpy(bytes.data(), field_bytes.data(), copy_n);
        const bool all_zero = std::all_of(bytes.begin(), bytes.end(), [](unsigned char b) { return b == 0; });
        if (all_zero) return zero_union();
        std::string arr = "[";
        for (size_t i = 0; i < bytes.size(); ++i) {
          if (i) arr += ", ";
          arr += "i8 " + std::to_string(static_cast<unsigned int>(bytes[i]));
        }
        arr += "]";
        return "{ [" + std::to_string(sz) + " x i8] " + arr + " }";
      };

      if (const auto* list = std::get_if<InitList>(&init)) {
        if (!list->items.empty()) {
          const InitListItem& item0 = list->items.front();
          size_t idx = 0;
          bool matched_union_field = false;
          GlobalInit child_init = GlobalInit(std::monostate{});
          if (item0.field_designator) {
            const auto fit = std::find_if(
                sd.fields.begin(), sd.fields.end(),
                [&](const HirStructField& f) { return f.name == *item0.field_designator; });
            if (fit != sd.fields.end()) {
              idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
              child_init = child_init_of(item0);
              matched_union_field = true;
            }
          } else if (item0.index_designator && *item0.index_designator >= 0) {
            idx = static_cast<size_t>(*item0.index_designator);
            child_init = child_init_of(item0);
            matched_union_field = true;
          }
          if (!matched_union_field) {
            idx = 0;
            child_init = (list->items.size() == 1 &&
                          std::holds_alternative<std::shared_ptr<InitList>>(item0.value))
                ? GlobalInit(*std::get<std::shared_ptr<InitList>>(item0.value))
                : GlobalInit(*list);
          }
          if (auto out = emit_union_from_field(idx, child_init)) return *out;
        }
        return zero_union();
      }

      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        const Expr& se = emitter_.get_expr(scalar->expr);
        long long val = 0;
        if (const auto* il = std::get_if<IntLiteral>(&se.payload)) val = il->value;
        else if (const auto* cl = std::get_if<CharLiteral>(&se.payload)) val = cl->value;
        if (val != 0) {
          std::string bytes(static_cast<size_t>(sz), '\0');
          for (int i = 0; i < sz; ++i) {
            bytes[static_cast<size_t>(i)] = static_cast<char>((val >> (8 * i)) & 0xFF);
          }
          return "{ [" + std::to_string(sz) + " x i8] c\"" + escape_llvm_c_bytes(bytes) + "\" }";
        }
      }
      return zero_union();
    }

    std::string format_array_literal(const TypeSpec& elem_ts, const std::vector<std::string>& elems) const {
      std::string out = "[";
      const std::string elem_ty = llvm_alloca_ty(elem_ts);
      for (size_t i = 0; i < elems.size(); ++i) {
        if (i) out += ", ";
        out += elem_ty + " " + elems[i];
      }
      out += "]";
      return out;
    }

    std::string format_struct_literal(const HirStructDef& sd,
                                      const std::vector<std::string>& field_vals) const {
      bool has_bitfields = false;
      for (const auto& f : sd.fields) {
        if (f.bit_width >= 0) {
          has_bitfields = true;
          break;
        }
      }
      if (!has_bitfields) {
        std::string out = "{ ";
        for (size_t i = 0; i < sd.fields.size(); ++i) {
          if (i) out += ", ";
          out += llvm_field_ty(sd.fields[i]) + " " + field_vals[i];
        }
        out += " }";
        return out;
      }

      std::string out = "{ ";
      bool first = true;
      int last_idx = -1;
      for (size_t i = 0; i < sd.fields.size();) {
        const auto& field = sd.fields[i];
        if (field.llvm_idx == last_idx) {
          ++i;
          continue;
        }
        last_idx = field.llvm_idx;
        if (!first) out += ", ";
        first = false;
        out += llvm_field_ty(field) + " ";
        if (field.bit_width < 0) {
          out += field_vals[i];
          ++i;
          continue;
        }
        unsigned long long packed = 0;
        while (i < sd.fields.size() && sd.fields[i].llvm_idx == last_idx) {
          const auto& bf = sd.fields[i];
          if (bf.bit_width > 0) {
            long long val = 0;
            try { val = std::stoll(field_vals[i]); } catch (...) {}
            const unsigned long long mask =
                (bf.bit_width >= 64) ? ~0ULL : ((1ULL << bf.bit_width) - 1);
            packed |= (static_cast<unsigned long long>(val) & mask) << bf.bit_offset;
          }
          ++i;
        }
        out += std::to_string(static_cast<long long>(packed));
      }
      out += " }";
      return out;
    }

    long long deduce_array_size_from_init(const GlobalInit& init) const {
      if (const auto* list = std::get_if<InitList>(&init)) {
        long long max_idx = -1;
        long long next = 0;
        for (const auto& item : list->items) {
          long long idx = next;
          if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
          if (idx > max_idx) max_idx = idx;
          next = idx + 1;
        }
        return max_idx + 1;
      }
      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        const Expr& e = emitter_.get_expr(scalar->expr);
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
          return static_cast<long long>(bytes_from_string_literal(*sl).size()) + 1;
        }
      }
      return -1;
    }

  };

std::vector<std::string> HirEmitter::emit_const_struct_fields(const TypeSpec& ts,
                                                              const HirStructDef& sd,
                                                              const GlobalInit& init,
                                                              std::vector<TypeSpec>* out_field_types) {
  return ConstInitEmitter(*this).emit_const_struct_fields(ts, sd, init, out_field_types);
}

std::string HirEmitter::emit_const_init(const TypeSpec& ts, const GlobalInit& init) {
  return ConstInitEmitter(*this).emit_const_init(ts, init);
}

}  // namespace tinyc2ll::codegen::llvm_backend
