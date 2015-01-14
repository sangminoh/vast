#include "vast/actor/tracker.h"
#include "vast/actor/identifier.h"
#include "vast/actor/replicator.h"

#include <caf/all.hpp>

using namespace caf;

namespace vast {

tracker::tracker(path dir)
  : dir_{std::move(dir)}
{
  trap_exit(true);
  attach_functor(
      [=](uint32_t)
      {
        identifier_ = invalid_actor;
      });
}

void tracker::at_down(caf::down_msg const& msg)
{
  for (auto i = actors_.begin(); i != actors_.end(); ++i)
    if (i->second.actor == msg.source)
    {
      VAST_INFO(this, "got DOWN from", i->first);
      i->second.actor = invalid_actor;
      //auto j = topology_.begin();
      //while (j != topology_.end())
      //  if (j->first == i->first || j->second == i->first)
      //  {
      //    VAST_VERBOSE(this, "removes link", j->first, "->", j->second);
      //    j = topology_.erase(j);
      //  }
      //  else
      //  {
      //    ++j;
      //  }
      break;
    }
}

void tracker::at_exit(caf::exit_msg const& msg)
{
  for (auto& p : actors_)
    send_exit(p.second.actor, msg.reason);
  quit(msg.reason);
}

message_handler tracker::make_handler()
{
  identifier_ = spawn<identifier, linked>(dir_);

  return
  {
    on(atom("identifier")) >> [=]
    {
      return identifier_;
    },
    on(atom("put"), arg_match)
      >> [=](std::string const& type, actor const& a, std::string const& name)
    {
      auto c = component::invalid;
      if (type == "importer")
        c = component::importer;
      else if (type == "exporter")
        c = component::exporter;
      else if (type == "receiver")
        c = component::receiver;
      else if (type == "archive")
        c = component::archive;
      else if (type == "index")
        c = component::index;
      else if (type == "search")
        c = component::search;
      else
        return make_message(error{"invalid type: ", type});

      auto i = actors_.find(name);
      if (i == actors_.end())
      {
        VAST_INFO(this, "registers", type << ':', name);
        actors_.emplace(name, actor_state{a, c});
      }
      else
      {
        if (i->second.type != c)
        {
          VAST_WARN(this, "found existing actor with different type:", name);
          return make_message(error{"type mismatch for: ", name});
        }
        if (i->second.actor != invalid_actor)
        {
          VAST_WARN(this, "got duplicate actor:", name);
          return make_message(error{"duplicate actor: ", name});
        }
        VAST_INFO(this, "re-instantiates", name);
        i->second.actor = a;
      }

      monitor(a);
      return make_message(atom("ok"));
    },
    on(atom("get"), arg_match) >> [=](std::string const& name)
    {
      auto i = actors_.find(name);
      if (i != actors_.end())
        return make_message(i->second.actor);
      else
        return make_message(error{"unknown actor: ", name});
    },
    on(atom("link"), arg_match)
      >> [=](std::string const& source, std::string const& sink)
    {
      auto i = actors_.find(source);
      actor_state* src = nullptr;
      if (i != actors_.end())
        src = &i->second;
      else
        return make_message(error{"unknown source: ", source});

      i = actors_.find(sink);
      actor_state* snk = nullptr;
      if (i != actors_.end())
        snk = &i->second;
      else
        return make_message(error{"unknown sink: ", sink});

      auto er = topology_.equal_range(source);
      for (auto i = er.first; i != er.second; ++i)
        if (i->first == source && i->second == sink)
        {
          VAST_DEBUG(this, "ignores existing link: ", source, " -> ", sink);
          return make_message(atom("ok"));
        }

      VAST_VERBOSE(this, "links", source, "->", sink);

      scoped_actor self;
      message_handler ok = on(atom("ok")) >> [] { /* do nothing */ };
      switch (src->type)
      {
        default:
          return make_message(error{"invalid source: ", source});
        case component::importer:
          if (snk->type != component::receiver)
            return make_message(error{"sink not a receiver: ", sink});
          else
            self->sync_send(src->actor, atom("add"), atom("sink"),
                            snk->actor).await(ok);
          break;
        case component::receiver:
        case component::search:
          if (snk->type == component::archive)
            self->sync_send(src->actor, atom("add"), atom("archive"),
                            snk->actor).await(ok);
          else if (snk->type == component::index)
            self->sync_send(src->actor, atom("add"), atom("index"),
                            snk->actor).await(ok);
          else
            return make_message(error{"sink not archive or index: ", sink});
          break;
      }

      topology_.emplace(source, sink);
      return make_message(atom("ok"));
    }
  };
}

std::string tracker::name() const
{
  return "tracker";
}

} // namespace vast