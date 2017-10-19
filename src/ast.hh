
#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "type.hh"

class Expr {
 public:
  virtual std::string toString() { return "[expr]"; }
  virtual void accept(class Visitor * v);
  virtual ~Expr() { }
};

#define EXPR_NODE_LIST(M) /*
*/ M(Expr) /*
*/ M(BinOp) /*
*/ M(Call) /*
*/ M(Index) /*
*/ M(Extern) /*
*/ M(String) /*
*/ M(Long) /*
*/ M(Double) /*
*/ M(Module) /*
*/ M(FuncDef) /*
*/ M(BlockExpr) /*
*/ M(Var) /*
*/ M(If) /*
*/ M(For) /*
*/ M(Return) /*
*/ M(Cast) /*
*/ M(Let) /*
*/ M(AddrOf) /*
*/ M(DeclClass) /*
*/ M(ImplType) /*
*/ M(ImplClassFor) /*
*/ M(DeclTypeEnum) /*
*/ M(DeclTypeAlias) /*
*/ M(MatchExpr) /*
*/ M(MatchCaseTagsExpr) /*
*/ M(EnumCase)

class Call : public Expr {
 public:
  Expr * callee;
  std::vector<Expr *> arguments;
 Call(Expr * a, std::vector<Expr *> b) : callee(a), arguments(b) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() {
    auto v = callee->toString() + "(";
    for(auto i = arguments.begin(); i != arguments.end(); i++) {
      if (*i) v += (i != arguments.begin() ? ", " : "") + (*i)->toString();
      else v = v + (i != arguments.begin() ? ", " : "")  +  "(null)";
    }
    v += ")";
    return v;
  }
  ~Call() { delete callee; for(auto i = arguments.begin(); i < arguments.end(); i++) { delete *i; }  }
};

class Index : public Expr {
public:
  Expr *base;
  vector <Expr *> indices;
  Index(Expr * base, vector<Expr *> indices) : base(base), indices(indices) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() {
    auto v = base->toString() + "[";
    for(auto i = indices.begin(); i != indices.end(); i++) {
      v += (i != indices.begin() ? ", " : "");
      if (*i) v += (*i)->toString();
      else v += "(null)";
    }
    v += "]";
    return v;
  }
};

class BinOp : public Expr {
 public:
  std::string op;
  Expr * lhs, *rhs;
  BinOp(std::string oper, Expr * a, Expr * b) : op(oper), lhs(a), rhs(b) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() {
    if (!lhs || !rhs)
      return (
	      "(" + (lhs ? lhs->toString() : "??") +
	      "|" + (rhs ? rhs->toString() : "??") +
	      "|" + op + ")"
	      );
    return "(" + lhs->toString() + op + rhs->toString() + ")";
  }
    ~BinOp() { delete lhs; delete rhs; }
};

class Extern : public Expr {
 public :
  std::string linkage;
  std::string name;
  Type *type;
  Extern(std::string a, std::string b, Type * type) :
    linkage(a), name(b), type(type) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "extern " + linkage + " " + name; }
  ~Extern() { delete type; }
};

class String : public Expr {
 public :
  std::string value;
 String(std::string a) : value(a) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
};

class Var : public Expr {
 public :
  std::string value;
  Var(std::string a) : value(a) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
};

class Double : public Expr {
 public :
  double value;
 Double(double a) : value(a) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
};

class Long : public Expr {
 public :
  int64_t value;
 Long(int64_t a) : value(a) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
};

class Module : public Expr {
 public:
  std::string name;
  std::vector<Expr *> lines;
  Module(std::vector<Expr *> a) : name("module"), lines(a) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
  ~Module() {
  for(auto i = lines.begin(), e = lines.end(); i != e; i++) delete *i; }
};

class BlockExpr : public Expr {
 public:
  std::vector<Expr *>lines;
 BlockExpr(std::vector<Expr *>lines) : lines(lines) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
  ~BlockExpr() { for(auto i = lines.begin(), e = lines.end(); i != e; i++) delete *i; }
};

class Def {
 public:
  std::string name;
  Type * type;
  Def(std::string name, Type * t) : name(name), type(t) { }
  ~Def() { delete type; }
};

class FuncDef : public Expr {
 public:
  std::string name;
  Type * rettype;
  std::vector<Def *> args;
  Expr * body;
  bool multi;
  FuncDef(std::string name, Type * rettype,
	  std::vector<Def *> args,
	  Expr * body,
	  bool multi) :
	      name(name), rettype(rettype), args(args), body(body), multi(multi) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
  ~FuncDef() {
    if (body) { delete body; }
    // if (rettype) { delete rettype; }
    for(auto i = args.begin(), e = args.end(); i != e; i++) if (*i) { delete *i; }
  }
};

class Cast : public Expr {
public:
  Expr * expr;
  Type * to_type;
  Cast(Expr * expr, Type * to_type) : expr(expr), to_type(to_type) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
  ~Cast() { delete expr; delete to_type; }
};

class If : public Expr {
public:
  Expr * cond, *ifbody, *elsebody;
  If(Expr * cond, Expr * ifbody, Expr * elsebody) : cond(cond), ifbody(ifbody), elsebody(elsebody) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
  ~If() { delete cond; delete ifbody; delete elsebody; }
};

class For : public Expr {
public:
  vector<Def *> var;
  Expr *source, *body;
  For(vector<Def *> var, Expr * source, Expr * body) : var(var), source(source), body(body) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "for"; }
  ~For() { delete source; delete body; }
};

class Let : public Expr {
public:
  Def * var;
  Expr * value;
  Let(Def * var, Expr * value) : var(var), value(value) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
  ~Let() { delete var; delete value; }
};

class AddrOf : public Expr {
public:
  std::string var;
  AddrOf(std::string var) : var(var) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
};

class Return : public Expr {
public:
  Expr * value;
  Return(Expr * value) : value(value) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString();
  ~Return() { delete value; }
};

class DeclTypeAlias : public Expr {
public:
  std::string name;
  Type * wrapped;
  DeclTypeAlias(std::string name, Type * wrapped)
    : name(name), wrapped(wrapped) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "type-alias"; }
  ~DeclTypeAlias() { delete wrapped; }
};

class DeclTypeEnum : public Expr {
public:
  std::string name;
  std::vector<Expr *> body;
  DeclTypeEnum(
    std::string name,
    std::string unused,
    std::vector<Expr *> body
    ) : name(name), body(body) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "type-enum"; }
  ~DeclTypeEnum() { }
};

class EnumCase : public Expr {
public:
  std::string name;
  std::vector<Def *> defs;
  EnumCase(std::string name) : name(name) { }
  EnumCase(std::string name, std::vector<Def *> defs)
    :name(name), defs(defs) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "enum-case"; }
};

class MatchExpr : public Expr {
public:
  Expr * cond;
  std::vector<Expr *> cases;
  MatchExpr(Expr * cond, std::vector<Expr *> cases)
    : cond(cond), cases(cases) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "enum-case"; }
};

class MatchCaseExpr : public Expr {
};
class MatchCaseTagsExpr : public MatchCaseExpr {
public:
  Expr * label;
  Expr * body;
  MatchCaseTagsExpr(Expr * label, Expr * body)
    : label(label), body(body) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "match-case"; }
  ~MatchCaseTagsExpr() { delete label; delete body; }
};

class DeclClass : public Expr {
public:
  std::string name;
  std::vector<Def *> lines;
  DeclClass(std::string name, std::vector<Def *> lines)
    : name(name), lines(lines) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "class-def-" + name; }
};

class ImplClassFor : public Expr {
public:
  std::string type_name;
  std::string class_name;
  Expr * body;
  ImplClassFor(
    std::string classname,
    std::string name,
    Expr * body)
    : type_name(name), class_name(classname), body(body) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() {
    return "impl-class-for-" + type_name + "-" + class_name; }
};

class ImplType : public Expr {
public:
  std::string name;
  Expr * body;
  ImplType(std::string name, Expr * body)
    : name(name), body(body) { }
  virtual void accept(class Visitor * v);
  virtual std::string toString() { return "impl-" + name; }
};


class Visitor {
 public:
#define VISIT(NODE) virtual void visit(NODE * c) { std::cerr << "visit: " << #NODE << std::endl; }
  EXPR_NODE_LIST(VISIT)
#undef VISIT
  virtual ~Visitor () { }
};

// useful visitors go here
#define EXPRNAME(t) ExprNameVisitor(t).out
class ExprNameVisitor : public Visitor {
public:
  std::string out;
  ExprNameVisitor(Expr * e) { e->accept(this); }
#define VISIT(NODE) virtual void visit(NODE * c) { out = #NODE; }
EXPR_NODE_LIST(VISIT)
#undef VISIT
};


class NameGetter : public Visitor {
public:
  string out;
  void visit(Var * v) { out = v->value; }
  void visit(Expr * e) { out = ""; }
};

string getName(Expr * e);

FuncDef* BuildFunc(std::string name, Type* functype, std::vector<Def *> params, Expr * body);
FuncDef* BuildVarFunc(std::string name, Type* functype, std::vector<Def *> params, Expr * body);
