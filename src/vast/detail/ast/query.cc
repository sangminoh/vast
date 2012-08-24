#include "vast/detail/ast/query.h"

#include <boost/variant/apply_visitor.hpp>

namespace vast {
namespace detail {
namespace ast {
namespace query {

struct folder : public boost::static_visitor<ze::value>
{
  static ze::value apply(expr_operator op, ze::value const& val)
  {
    switch (op)
    {
      default:
        assert(! "unary expression folder not yet implemented");
        return ze::invalid;
        // TODO: implement 0event operations.
        //case positive:
        //    return val;
        //case negative:
        //    return -val;
        //case bitwise_not:
        //    return ~val;
    }
  }

  static ze::value apply(expr_operator op,
                         ze::value const& lhs,
                         ze::value const& rhs)
  {
    switch (op)
    {
      default:
        assert(! "binary expression folder not yet implemented");
        return ze::invalid;
        // TODO: implement 0event operations.
        //case bitwise_or:
        //    return lhs | rhs;
        //case bitwise_xor:
        //    return lhs ^ rhs;
        //case bitwise_and:
        //    return lhs & rhs;
        //case plus:
        //    return lhs + rhs;
        //case minus:
        //    return lhs - rhs;
        //case times:
        //    return lhs * rhs;
        //case divide:
        //    return lhs / rhs;
        //case mod:
        //    return lhs % rhs;
    }
  }

  ze::value operator()(ze::value const& val) const
  {
    return val;
  }

  ze::value operator()(unary_expr const& unary) const
  {
    auto operand = boost::apply_visitor(*this, unary.operand);
    return apply(unary.op, operand);
  }

  ze::value operator()(expr_operand const& operand) const
  {
    return boost::apply_visitor(*this, operand);
  }

  ze::value operator()(expression const& expr) const
  {
    auto value = boost::apply_visitor(*this, expr.first);
    if (expr.rest.empty())
      return value;

    for (auto& operation : expr.rest)
    {
      auto operand = boost::apply_visitor(*this, operation.operand);
      value = apply(operation.op, value, operand);
    }

    return value;
  }
};

struct validator : public boost::static_visitor<bool>
{
  bool operator()(clause const& operand) const
  {
    return boost::apply_visitor(*this, operand);
  }

  bool operator()(tag_clause const& clause) const
  {
    auto rhs = fold(clause.rhs);
    auto rhs_type = rhs.which();
    auto& lhs = clause.lhs;
    return
      (lhs == "name" && (rhs_type == ze::string_type
                         || rhs_type == ze::regex_type))
      || (lhs == "time" && rhs_type == ze::time_point_type)
      || (lhs == "id" && rhs_type == ze::uint_type);
  }

  bool operator()(type_clause const& clause) const
  {
    auto rhs = fold(clause.rhs);
    auto rhs_type = rhs.which();
    auto& lhs_type = clause.lhs;
    auto& op = clause.op;
    return
      lhs_type == rhs_type
      || (lhs_type == ze::string_type
          && (op == match || op == not_match || op == in || op == not_in)
          && rhs_type == ze::regex_type)
      || (lhs_type == ze::address_type && clause.op == in
          && rhs_type == ze::prefix_type);
  }

  bool operator()(offset_clause const& clause) const
  {
    auto rhs = fold(clause.rhs);
    return ! clause.offsets.empty();
  }

  bool operator()(event_clause& clause) const
  {
    // TODO: Perform the dereference and replace it with the corresponding
    // offset.
    clause.lhs[1] = std::to_string(0ul);
    clause.lhs.resize(2);

    return true;
  }

  bool operator()(negated_clause const& clause) const
  {
    return boost::apply_visitor(*this, clause.operand);
  }
};

clause_operator negate(clause_operator op)
{
  switch (op)
  {
    default:
      assert(! "missing operator implementation");
    case match:
      return not_match;
    case not_match:
      return match;
    case equal:
      return not_equal;
    case not_equal:
      return equal;
    case less:
      return greater_equal;
    case less_equal:
      return greater;
    case greater:
      return less_equal;
    case greater_equal:
      return less;
    case in:
      return not_in;
    case not_in:
      return in;
  }
}

ze::value fold(expression const& expr)
{
  auto value = boost::apply_visitor(folder(), expr.first);
  if (expr.rest.empty())
    return value;

  for (auto& operation : expr.rest)
  {
    auto operand = boost::apply_visitor(folder(), operation.operand);
    value = folder::apply(operation.op, value, operand);
  }

  return value;
}

bool validate(query& q)
{
  if (! boost::apply_visitor(validator(), q.first))
    return false;

  for (auto& operation : q.rest)
    if (! boost::apply_visitor(validator(), operation.operand))
      return false;

  return true;
};

} // namespace query
} // namespace ast
} // namespace detail
} // namespace vast