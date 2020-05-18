#include <deip/protocol/percent.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>

/*

The bounds on asset serialization are as follows:

index : field
0     : decimals
1..6  : symbol
   7  : \0
*/

namespace deip {
namespace protocol {
typedef boost::multiprecision::int128_t int128_t;


int64_t percent::precision(uint8_t d)
{
    static int64_t table[] = { 1,
                               10,
                               100,
                               1000,
                               10000,
                               100000,
                               1000000,
                               10000000,
                               100000000ll,
                               1000000000ll,
                               10000000000ll,
                               100000000000ll,
                               1000000000000ll,
                               10000000000000ll,
                               100000000000000ll };

    return table[d];
}

string percent::to_string() const
{
    int64_t prec = precision(decimals);
    string result = fc::to_string(amount.value / prec);
    if (prec > 1)
    {
        auto fract = amount.value % prec;
        // prec is a power of ten, so for example when working with
        // 7.005 we have fract = 5, prec = 1000.  So prec+fract=1005
        // has the correct number of zeros and we can simply trim the
        // leading 1.
        result += "." + fc::to_string(prec + fract).erase(0, 1);
    }
    return result + " " + std::string(1, symbol);
}

percent percent::from_string(const string& from)
{
    try
    {
        string s = fc::trim(from);
        auto space_pos = s.find(" ");
        auto dot_pos = s.find(".");
        
        FC_ASSERT(space_pos != std::string::npos);

        auto percent_symbol = std::string(s.substr(space_pos + 1));
        FC_ASSERT(percent_symbol == std::string(1, symbol));

        if (dot_pos != std::string::npos)
        {
            FC_ASSERT(space_pos > dot_pos);

            auto intpart = s.substr(0, dot_pos);
            auto fractpart = "1" + s.substr(dot_pos + 1, space_pos - dot_pos - 1);
            
            uint8_t d = fractpart.size() - 1;
            FC_ASSERT(d < 15);
            auto prec = precision(d);

            auto amount = share_type(fc::to_int64(intpart));
            amount.value *= prec;
            amount.value += fc::to_int64(fractpart);
            amount.value -= prec;

            percent result = percent(amount, d);
            return result;
        }
        else
        {
            auto intpart = s.substr(0, space_pos);
            auto amount = share_type(fc::to_int64(intpart));

            percent result = percent(amount, 0);
            return result;
        }

    }
    FC_CAPTURE_AND_RETHROW((from))
}





} // namespace protocol
} // namespace deip