#include <deip/tags/tags_plugin.hpp>

#include <deip/app/impacted.hpp>

#include <deip/protocol/config.hpp>

#include <deip/chain/database.hpp>
#include <deip/chain/hardfork.hpp>
#include <deip/chain/operation_notification.hpp>
#include <deip/chain/account_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/json.hpp>
#include <fc/string.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

namespace deip {
namespace tags {

namespace detail {

using namespace deip::protocol;

class tags_plugin_impl
{
public:
    tags_plugin_impl(tags_plugin& _plugin)
        : _self(_plugin)
    {
    }
    virtual ~tags_plugin_impl();

    deip::chain::database& database()
    {
        return _self.database();
    }

    void on_operation(const operation_notification& note);

    tags_plugin& _self;
};

tags_plugin_impl::~tags_plugin_impl()
{
    return;
}

struct operation_visitor
{
    operation_visitor(database& db)
        : _db(db){};
    typedef void result_type;

    database& _db;

    void remove_stats(const tag_object& tag, const tag_stats_object& stats) const
    {
        _db.modify(stats, [&](tag_stats_object& s) {
            if (tag.parent == comment_id_type())
            {
                s.top_posts--;
            }
            else
            {
                s.comments--;
            }
            s.total_trending -= static_cast<uint32_t>(tag.trending);
            s.net_votes -= tag.net_votes;
        });
    }

    void add_stats(const tag_object& tag, const tag_stats_object& stats) const
    {
        _db.modify(stats, [&](tag_stats_object& s) {
            if (tag.parent == comment_id_type())
            {
                s.top_posts++;
            }
            else
            {
                s.comments++;
            }
            s.total_trending += static_cast<uint32_t>(tag.trending);
            s.net_votes += tag.net_votes;
        });
    }

    void remove_tag(const tag_object& tag) const
    {
        /// TODO: update tag stats object
        _db.remove(tag);

        const auto& idx = _db.get_index<author_tag_stats_index>().indices().get<by_author_tag_posts>();
        auto itr = idx.lower_bound(boost::make_tuple(tag.author, tag.tag));
        if (itr != idx.end() && itr->author == tag.author && itr->tag == tag.tag)
        {
            _db.modify(*itr, [&](author_tag_stats_object& stats) { stats.total_posts--; });
        }
    }

    const tag_stats_object& get_stats(const string& tag) const
    {
        const auto& stats_idx = _db.get_index<tag_stats_index>().indices().get<by_tag>();
        auto itr = stats_idx.find(tag);
        if (itr != stats_idx.end())
            return *itr;

        return _db.create<tag_stats_object>([&](tag_stats_object& stats) { stats.tag = tag; });
    }

    /**
     * https://medium.com/hacking-and-gonzo/how-reddit-ranking-algorithms-work-ef111e33d0d9#.lcbj6auuw
     */
    template <int64_t S, int32_t T> double calculate_score(const share_type& score, const time_point_sec& created) const
    {
        /// new algorithm
        auto mod_score = score.value / S;

        /// reddit algorithm
        double order = log10(std::max<int64_t>(std::abs(mod_score), 1));
        int sign = 0;
        if (mod_score > 0)
            sign = 1;
        else if (mod_score < 0)
            sign = -1;

        return sign * order + double(created.sec_since_epoch()) / double(T);
    }

    inline double calculate_hot(const share_type& score, const time_point_sec& created) const
    {
        return calculate_score<10000000, 10000>(score, created);
    }

    inline double calculate_trending(const share_type& score, const time_point_sec& created) const
    {
        return calculate_score<10000000, 480000>(score, created);
    }

    /** finds tags that have been added or removed or updated */
    const peer_stats_object& get_or_create_peer_stats(account_id_type voter, account_id_type peer) const
    {
        const auto& peeridx = _db.get_index<peer_stats_index>().indices().get<by_voter_peer>();
        auto itr = peeridx.find(boost::make_tuple(voter, peer));
        if (itr == peeridx.end())
        {
            return _db.create<peer_stats_object>([&](peer_stats_object& obj) {
                obj.voter = voter;
                obj.peer = peer;
            });
        }
        return *itr;
    }

    void update_indirect_vote(account_id_type a, account_id_type b, int positive) const
    {
        if (a == b)
            return;
        const auto& ab = get_or_create_peer_stats(a, b);
        const auto& ba = get_or_create_peer_stats(b, a);
        _db.modify(ab, [&](peer_stats_object& o) {
            o.indirect_positive_votes += positive;
            o.indirect_votes++;
            o.update_rank();
        });
        _db.modify(ba, [&](peer_stats_object& o) {
            o.indirect_positive_votes += positive;
            o.indirect_votes++;
            o.update_rank();
        });
    }

    void operator()(const vote_operation& op) const
    {
        //update_tags(_db.get_comment(op.author, op.permlink));
        /*
        update_peer_stats( _db.get_account(op.voter),
                           _db.get_account(op.author),
                           _db.get_comment(op.author, op.permlink),
                           op.weight );
                           */
    }

    template <typename Op> void operator()(Op&&) const
    {
    } /// ignore all other ops
};

void tags_plugin_impl::on_operation(const operation_notification& note)
{
    try
    {
        /// plugins shouldn't ever throw
        note.op.visit(operation_visitor(database()));
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
    }
    catch (...)
    {
        elog("unhandled exception");
    }
}

} /// end detail namespace

tags_plugin::tags_plugin(application* app)
    : plugin(app)
    , my(new detail::tags_plugin_impl(*this))
{
    chain::database& db = database();

    db.add_plugin_index<tag_index>();
    db.add_plugin_index<tag_stats_index>();
    db.add_plugin_index<peer_stats_index>();
    db.add_plugin_index<author_tag_stats_index>();
}

tags_plugin::~tags_plugin()
{
}

void tags_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                             boost::program_options::options_description& cfg)
{
}

void tags_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    ilog("Intializing tags plugin");
    database().post_apply_operation.connect([&](const operation_notification& note) { my->on_operation(note); });

    app().register_api_factory<tag_api>("tag_api");
}

void tags_plugin::plugin_startup()
{
}
}
} /// deip::tags

DEIP_DEFINE_PLUGIN(tags, deip::tags::tags_plugin)
