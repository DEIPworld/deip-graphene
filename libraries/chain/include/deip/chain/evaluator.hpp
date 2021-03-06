#pragma once
#include <deip/protocol/exceptions.hpp>
#include <deip/protocol/operations.hpp>

namespace deip {
namespace chain {

class dbservice;

template <typename OperationType = deip::protocol::operation> class evaluator
{
public:
    virtual ~evaluator() = 0;
    virtual void apply(const OperationType& op) = 0;
    virtual int get_type() const = 0;
};

template <typename OperationType>
evaluator<OperationType>::~evaluator()
{
}

template <typename EvaluatorType, typename OperationType = deip::protocol::operation>
class evaluator_impl : public evaluator<OperationType>
{
public:
    typedef OperationType operation_sv_type;
    // typedef typename EvaluatorType::operation_type op_type;

    evaluator_impl(dbservice& d)
        : _db(d)
    {
    }

    virtual void apply(const OperationType& o) final override
    {
        auto* eval = static_cast<EvaluatorType*>(this);
        const auto& op = o.template get<typename EvaluatorType::operation_type>();
        eval->do_apply(op);
    }

    virtual int get_type() const override
    {
        return OperationType::template tag<typename EvaluatorType::operation_type>::value;
    }

    dbservice& db()
    {
        return _db;
    }

protected:
    dbservice& _db;
};
}
}

#define DEFINE_EVALUATOR(X)                                                                                            \
    class X##_evaluator : public deip::chain::evaluator_impl<X##_evaluator>                                          \
    {                                                                                                                  \
    public:                                                                                                            \
        typedef X##_operation operation_type;                                                                          \
                                                                                                                       \
        X##_evaluator(dbservice& db)                                                                                   \
            : deip::chain::evaluator_impl<X##_evaluator>(db)                                                         \
        {                                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        void do_apply(const X##_operation& o);                                                                         \
    };
