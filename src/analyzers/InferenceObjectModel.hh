#include "core/expr.hh"
#include <iostream>
#include <vector>

namespace frobnob {
  // A TypeTerm is a variable we're solving for in the constraint system
  class TypeTerm {
  public:
    std::string name;
    coral::ast::BaseExpr * expr;
    TypeTerm(std::string name, coral::ast::BaseExpr * expr) : name(name), expr(expr) { }
  };

  // A typeconstraint is a formula on TypeTerms
  class TypeConstraint {
  public:
    virtual void print_to(std::ostream& out) { out << "(constraint)"; }
    virtual TypeConstraint * findTerm(__attribute__((unused)) TypeTerm * t) { return 0; }
    virtual TypeConstraint * replaceTerm(
      __attribute__((unused)) TypeTerm * tt,
      __attribute__((unused)) TypeConstraint * tc) { return this; }
  };
  class Term;
  class Type;
  class Call;
  class FreeType;
  class And;
  class Equal;

  std::ostream & operator<<(std::ostream &out, TypeTerm &tptr);
  std::ostream & operator<<(std::ostream &out, TypeTerm * tptr);
  std::ostream & operator<<(std::ostream &out, TypeConstraint &tc);
  std::ostream & operator<<(std::ostream &out, TypeConstraint *tc);
  std::ostream & operator<<(std::ostream &out, std::vector<TypeConstraint *> &vv);
  TypeConstraint * Global_Ops(std::string op);

  class Type : public TypeConstraint {
  public:
    std::string name;
    std::vector<std::unique_ptr<TypeConstraint>> params;
    Type(coral::type::Type basetype) : name(basetype.name) {
      for(auto &p: basetype.params)
        params.push_back(std::unique_ptr<Type>(new Type(p)));
    }
    Type(std::string name) : name(name) { }
    Type(std::string name, std::vector<TypeConstraint *> pp) : name(name) {
      for(auto &&p:pp) params.emplace_back(p);
    }
    coral::type::Type * concrete_type() {
      std::vector<coral::type::Type> params;
      bool is_valid = true;
      for(size_t i=0; i < this->params.size(); i++) {
        auto t = dynamic_cast<Type *>(this->params[i].get());
        if (!t) { is_valid = false; break; }
        auto ttype = t->concrete_type();
        if (!ttype) { is_valid = false; break; }
        else params.push_back(*ttype);
      }
      if (!is_valid) return 0;
      return new coral::type::Type(name, params);
    }
    virtual TypeConstraint * findTerm(TypeTerm * t) {
      for(auto && p: params)
        if(p->findTerm(t)) return this;
      return 0;
    }
    virtual TypeConstraint * replaceTerm(
      TypeTerm * tt,
      TypeConstraint * tc) {
      for(auto &p: params)
        p.reset(p.release()->replaceTerm(tt, tc));
      return this;
    }

    virtual void print_to(std::ostream& out) {
      out << name;
      if (params.size()) {
        out << "(";
        for(auto &&p: params) {
          if (&p != &params.front()) out << ", ";
          p->print_to(out);
        }
        out << ")"; }
    }
  };

  class Term : public TypeConstraint {
  public: TypeTerm * term;
    Term(TypeTerm * term) : term(term) { }
    virtual void print_to(std::ostream &out) {
      out << COL_RGB(5, 3, 4) << (term ? term->name : "(nullterm)") << COL_CLEAR;
    }
    virtual TypeConstraint * findTerm(TypeTerm * t) {
      if (this->term == t) return this;
      else return 0; }
    virtual TypeConstraint * replaceTerm(
      TypeTerm * tt,
      TypeConstraint * tc) {
      if (term == tt) return tc;
      return this;
    }
  };

  class Call : public TypeConstraint {
  public:
    TypeConstraint * callee;
    std::vector<TypeConstraint *> args;
    Call(TypeConstraint * callee, std::vector<TypeConstraint *> args)
      : callee(callee), args(args) { }
    virtual void print_to(std::ostream &out) {
      out << "Call(" << callee << ", " << args << ")";
    }
    virtual TypeConstraint * findTerm(TypeTerm * t) {
      auto out = callee->findTerm(t);
      if (out) return out;
      for(auto &arg: args) {
        out = arg->findTerm(t);
        if (out) return out;
      }
      return 0;
    }
    virtual TypeConstraint * replaceTerm(
      TypeTerm * tt,
      TypeConstraint * tc) {
      callee = callee->replaceTerm(tt, tc);
      for(auto &arg: args)
        arg = arg->replaceTerm(tt, tc);
      return this;
    }

  };

  class FreeType : public TypeConstraint {
  public:
    int id;
    FreeType() {
      static int maxid = 0;
      id = maxid++;
    }
    virtual void print_to(std::ostream &out) { out << "T" << id; }
  };

  class And : public TypeConstraint {
  public:
    std::vector<TypeConstraint *> terms;
    And(std::vector<TypeConstraint *> terms) : terms(terms) { }
    virtual void print_to(std::ostream & out) { out << "And(" << terms << ")"; }
  };

}