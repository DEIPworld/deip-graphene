#pragma once
#include <deip/protocol/types.hpp>
#include <deip/protocol/config.hpp>

namespace deip {
namespace protocol {

struct percent
{
    percent(share_type a = 0, uint8_t d = DEIP_PERCENT_DECIMALS)
        : amount(a)
        , decimals(d)
    {
        FC_ASSERT(decimals < 15);
    }

    share_type amount;
    uint8_t decimals;
    static const uint8_t symbol = uint8_t('%');

    double to_real() const
    {
        return double(amount.value) / precision(decimals);
    }

    static int64_t precision(uint8_t d);

    static percent from_string(const string& from);
    string to_string() const;

    percent& operator+=(const percent& o)
    {
        FC_ASSERT(decimals == o.decimals);
        amount += o.amount;
        return *this;
    }

    percent& operator-=(const percent& o)
    {
        FC_ASSERT(decimals == o.decimals);
        amount -= o.amount;
        return *this;
    }

    percent operator-() const
    {
        return percent(-amount, decimals);
    }

    friend bool operator==(const percent& a, const percent& b)
    {
        FC_ASSERT(a.decimals == b.decimals);
        return a.amount == b.amount;
    }

    friend bool operator<(const percent& a, const percent& b)
    {
        FC_ASSERT(a.decimals == b.decimals);
        return a.amount < b.amount;
    }

    friend bool operator<=(const percent& a, const percent& b)
    {
        return (a == b) || (a < b);
    }

    friend bool operator!=(const percent& a, const percent& b)
    {
        return !(a == b);
    }

    friend bool operator>(const percent& a, const percent& b)
    {
        return !(a <= b);
    }

    friend bool operator>=(const percent& a, const percent& b)
    {
        return !(a < b);
    }

    friend percent operator-(const percent& a, const percent& b)
    {
        FC_ASSERT(a.decimals == b.decimals);
        return percent(a.amount - b.amount, a.decimals);
    }

    friend percent operator+(const percent& a, const percent& b)
    {
        FC_ASSERT(a.decimals == b.decimals);
        return percent(a.amount + b.amount, a.decimals);
    }

    friend percent operator*(const percent& a, const percent& b)
    {
        FC_ASSERT(a.decimals == b.decimals);
        return percent(a.amount * b.amount, a.decimals);
    }
};

template <typename Stream> Stream& operator<<(Stream& stream, const deip::protocol::percent& a)
{
    stream << a.to_string();
    return stream;
}

template <typename Stream> Stream& operator>>(Stream& stream, deip::protocol::percent& a)
{
    std::string str;
    stream >> str;
    a = deip::protocol::percent::from_string(str);
    return stream;
}

bool operator<(const percent& a, const percent& b);
bool operator<=(const percent& a, const percent& b);

} // namespace protocol
} // namespace deip

namespace fc {

inline void to_variant(const deip::protocol::percent& var, fc::variant& vo)
{
    vo = var.to_string();
}

inline void from_variant(const fc::variant& var, deip::protocol::percent& vo)
{
    vo = deip::protocol::percent::from_string(var.as_string());
}

} // namespace fc

FC_REFLECT(deip::protocol::percent, (amount)(decimals))