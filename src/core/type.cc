#include "type.hh"

namespace coral {

  TypeKind getTypeKind(BaseType * t) { return TypeKindVisitor(t).out;  }

  std::string getTypeName(BaseType * t) { if (!t) return "(nulltype)"; return TypeNameVisitor(t).out; }

  std::ostream& operator << (std::ostream& os, BaseType * t) {
    os << (!t ? "(nulltype)" : t->toString());
    return os;
  }

  std::string BaseType::toString() { return TypeNameVisitor(this).out; }

#define ACCEPT(TYPE) void TYPE##Type::accept(TypeVisitor * t) { t->visit(this); }
  TYPE_LOOP(ACCEPT);
#undef ACCEPT

  Type * BuildType(std::string name) {
    // std::cerr << "buildling type " << name << std::endl;
    if (name == "Bool") return new IntType(1);
    if (name == "Int1") return new IntType(1);
    if (name == "Int8") return new IntType(8);
    if (name == "Int32") return new IntType(32);
    if (name == "Int64") return new IntType(64);
    if (name == "...") return new VariadicTypeInfo();
    if (name == "Void") return new VoidType();
    // std::cerr << "Unknown type: " << name << std::endl;
    return new UnknownType(name);
  }
  Type * BuildType(std::string name, std::vector<Type *> typeparams) {
    // std::cerr << "buildling type " << name << std::endl;
    if (name == "Ptr") return new PtrType(typeparams[0]);
    if (name == "Fn") {
      auto returnType = typeparams[typeparams.size()-1];
      auto params = std::vector<Type *>();
      for(int i=0; i<typeparams.size() - 1; i++)
	if (typeparams[i]->toString() != "...") params.push_back(typeparams[i]);
	else delete typeparams[i];
      return new FuncType(returnType, params, params.size() + 1 < typeparams.size());
    }
    std::cerr << "Unknown type: " << name << std::endl;
    return 0;
  }
}
