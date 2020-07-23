#pragma once
#include <deip/protocol/config.hpp>

#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/fixed_string.hpp>
#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>

#include <boost/multiprecision/cpp_int.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>

namespace deip {

using fc::uint128_t;
typedef boost::multiprecision::uint256_t u256;
typedef boost::multiprecision::uint512_t u512;

using std::deque;
using std::enable_shared_from_this;
using std::make_pair;
using std::map;
using std::pair;
using std::set;
using std::shared_ptr;
using std::string;
using std::tie;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using std::weak_ptr;

using fc::enum_type;
using fc::flat_map;
using fc::flat_set;
using fc::optional;
using fc::safe;
using fc::signed_int;
using fc::smart_ref;
using fc::static_variant;
using fc::time_point;
using fc::time_point_sec;
using fc::unsigned_int;
using fc::variant;
using fc::variant_object;
using fc::ecc::commitment_type;
using fc::ecc::range_proof_info;
using fc::ecc::range_proof_type;
struct void_t
{
};

namespace protocol {

typedef fc::ecc::private_key private_key_type;
typedef fc::sha256 chain_id_type;
typedef fc::fixed_string_40 account_name_type;
typedef fc::fixed_string_40 external_id_type;
typedef fc::ripemd160 block_id_type;
typedef fc::ripemd160 checksum_type;
typedef fc::ripemd160 transaction_id_type;
typedef fc::sha256 digest_type;
typedef fc::ecc::compact_signature signature_type;
typedef safe<int64_t> share_type;
typedef uint16_t weight_type;
typedef uint64_t asset_symbol_type;
typedef int64_t assessment_criteria_value;

struct public_key_type
{
    struct binary_key
    {
        binary_key()
        {
        }
        uint32_t check = 0;
        fc::ecc::public_key_data data;
    };
    fc::ecc::public_key_data key_data;
    public_key_type();
    public_key_type(const fc::ecc::public_key_data& data);
    public_key_type(const fc::ecc::public_key& pubkey);
    explicit public_key_type(const std::string& base58str);
    operator fc::ecc::public_key_data() const;
    operator fc::ecc::public_key() const;
    explicit operator std::string() const;
    friend bool operator==(const public_key_type& p1, const fc::ecc::public_key& p2);
    friend bool operator==(const public_key_type& p1, const public_key_type& p2);
    friend bool operator<(const public_key_type& p1, const public_key_type& p2)
    {
        return p1.key_data < p2.key_data;
    }
    friend bool operator!=(const public_key_type& p1, const public_key_type& p2);
};

struct extended_public_key_type
{
    struct binary_key
    {
        binary_key()
        {
        }
        uint32_t check = 0;
        fc::ecc::extended_key_data data;
    };

    fc::ecc::extended_key_data key_data;

    extended_public_key_type();
    extended_public_key_type(const fc::ecc::extended_key_data& data);
    extended_public_key_type(const fc::ecc::extended_public_key& extpubkey);
    explicit extended_public_key_type(const std::string& base58str);
    operator fc::ecc::extended_public_key() const;
    explicit operator std::string() const;
    friend bool operator==(const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2);
    friend bool operator==(const extended_public_key_type& p1, const extended_public_key_type& p2);
    friend bool operator!=(const extended_public_key_type& p1, const extended_public_key_type& p2);
};

struct extended_private_key_type
{
    struct binary_key
    {
        binary_key()
        {
        }
        uint32_t check = 0;
        fc::ecc::extended_key_data data;
    };

    fc::ecc::extended_key_data key_data;

    extended_private_key_type();
    extended_private_key_type(const fc::ecc::extended_key_data& data);
    extended_private_key_type(const fc::ecc::extended_private_key& extprivkey);
    explicit extended_private_key_type(const std::string& base58str);
    operator fc::ecc::extended_private_key() const;
    explicit operator std::string() const;
    friend bool operator==(const extended_private_key_type& p1, const fc::ecc::extended_private_key& p2);
    friend bool operator==(const extended_private_key_type& p1, const extended_private_key_type& p2);
    friend bool operator!=(const extended_private_key_type& p1, const extended_private_key_type& p2);
};

enum class authority_type : uint16_t
{
    unknown = 0,
    owner = 1,
    active = 2,

    FIRST = owner,
    LAST = active
};

typedef uint32_t percent_type;

} // namespace protocol
} // namespace deip

namespace fc {
void to_variant(const deip::protocol::public_key_type& var, fc::variant& vo);
void from_variant(const fc::variant& var, deip::protocol::public_key_type& vo);
void to_variant(const deip::protocol::extended_public_key_type& var, fc::variant& vo);
void from_variant(const fc::variant& var, deip::protocol::extended_public_key_type& vo);
void to_variant(const deip::protocol::extended_private_key_type& var, fc::variant& vo);
void from_variant(const fc::variant& var, deip::protocol::extended_private_key_type& vo);

std::string static_variant_name_from_type(const std::string& type_name);

struct from_static_variant_type
{
    variant& var;
    from_static_variant_type(variant& dv)
        : var(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        auto name = static_variant_name_from_type(fc::get_typename<T>::name());
        var = variant(std::make_pair(name, v));
    }
};

struct get_static_variant_type
{
    string& name;
    get_static_variant_type(string& dv)
        : name(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        name = static_variant_name_from_type(fc::get_typename<T>::name());
    }
};

} // namespace fc


#define DECLARE_STATIC_VARIANT_TYPE(StaticVariantType)                                                                 \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const StaticVariantType&, fc::variant&);                                                           \
    void from_variant(const fc::variant&, StaticVariantType&);                                                         \
                                                                                                                       \
    } /* fc */


#define DEFINE_STATIC_VARIANT_TYPE(StaticVariantType)                                                                  \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const StaticVariantType& var, fc::variant& vo)                                                     \
    {                                                                                                                  \
        var.visit(from_static_variant_type(vo));                                                                       \
    }                                                                                                                  \
                                                                                                                       \
    void from_variant(const fc::variant& var, StaticVariantType& vo)                                                   \
    {                                                                                                                  \
        static std::map<string, uint32_t> to_tag = []() {                                                              \
            std::map<string, uint32_t> name_map;                                                                       \
            for (int i = 0; i < StaticVariantType::count(); ++i)                                                       \
            {                                                                                                          \
                StaticVariantType tmp;                                                                                 \
                tmp.set_which(i);                                                                                      \
                string n;                                                                                              \
                tmp.visit(get_static_variant_type(n));                                                                 \
                name_map[n] = i;                                                                                       \
            }                                                                                                          \
            return name_map;                                                                                           \
        }();                                                                                                           \
                                                                                                                       \
        auto ar = var.get_array();                                                                                     \
        if (ar.size() < 2)                                                                                             \
            return;                                                                                                    \
        if (ar[0].is_uint64())                                                                                         \
            vo.set_which(ar[0].as_uint64());                                                                           \
        else                                                                                                           \
        {                                                                                                              \
            auto itr = to_tag.find(ar[0].as_string());                                                                 \
            FC_ASSERT(itr != to_tag.end(), "Invalid static variant type: ${n}", ("n", ar[0]));                         \
            vo.set_which(to_tag[ar[0].as_string()]);                                                                   \
        }                                                                                                              \
        vo.visit(fc::to_static_variant(ar[1]));                                                                        \
    }                                                                                                                  \
    } /* fc */


FC_REFLECT(deip::protocol::public_key_type, (key_data))
FC_REFLECT(deip::protocol::public_key_type::binary_key, (data)(check))
FC_REFLECT(deip::protocol::extended_public_key_type, (key_data))
FC_REFLECT(deip::protocol::extended_public_key_type::binary_key, (check)(data))
FC_REFLECT(deip::protocol::extended_private_key_type, (key_data))
FC_REFLECT(deip::protocol::extended_private_key_type::binary_key, (check)(data))

FC_REFLECT_TYPENAME(deip::protocol::share_type)

FC_REFLECT(deip::void_t, )

FC_REFLECT_ENUM(deip::protocol::authority_type, 
  (unknown)
  (owner)
  (active)
)