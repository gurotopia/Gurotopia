#include "pch.hpp"
#include <map>
#include "tools/create_dialog.hpp"
#include "database/items.hpp"
#include "database/world.hpp"
#include "growscan.hpp"

static constexpr u_short GROWSCAN_ID = 6016;

static void send_dialog(ENetEvent &event, ::create_dialog &d)
{
    send_varlist(event.peer, { "OnDialogRequest", d.end_dialog("growscan", "Close", "") });
}

/* adds one icon line per item, most first */
static void add_counts(::create_dialog &d, const std::map<u_short, int> &counts)
{
    std::vector<std::pair<u_short, int>> list(counts.begin(), counts.end());
    std::ranges::sort(list, [](const auto &a, const auto &b) { return a.second > b.second; });
    for (const auto &[id, n] : list)
        d.add_label_with_icon("small", std::format("`w{}``: {}", id_to_item(id).raw_name, n), id);
}

void growscan_open(ENetEvent &event, ::world &w)
{
    ::create_dialog d{};
    d.set_default_color("`o")
     .add_label_with_icon("big", "`wGrowscan 9000``", GROWSCAN_ID)
     .add_textbox(std::format("Scanning `w{}``. What would you like to see?", w.name))
     .add_button("gs_blocks", "World Blocks")
     .add_button("gs_floating", "Floating Items");
    send_dialog(event, d);
}

void growscan_return(ENetEvent &event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    auto w = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (w == worlds.end()) return;

    const std::string btn = hPipe["buttonClicked"];
    if (btn == "gs_back") { growscan_open(event, *w); return; }

    ::create_dialog d{};
    d.set_default_color("`o");

    if (btn == "gs_blocks")
    {
        std::map<u_short, int> fg{}, bg{};
        for (const ::block &b : w->blocks)
        {
            if (b.fg != 0) ++fg[b.fg];
            if (b.bg != 0) ++bg[b.bg];
        }
        d.add_label_with_icon("big", "`wWorld Blocks``", GROWSCAN_ID)
         .add_spacer("small").add_label("big", "`wBlocks``");
        add_counts(d, fg);
        d.add_spacer("small").add_label("big", "`wBackgrounds``");
        add_counts(d, bg);
    }
    else if (btn == "gs_floating")
    {
        std::map<u_short, int> floating{};
        for (const ::object &o : w->objects) floating[o.id] += o.count;
        d.add_label_with_icon("big", "`wFloating Items``", GROWSCAN_ID)
         .add_smalltext(std::format("`w{}`` drops lying around.", w->objects.size()));
        if (floating.empty()) d.add_textbox("Nothing is floating in this world.");
        else add_counts(d, floating);
    }
    else return;

    d.add_spacer("small").add_button("gs_back", "Back");
    send_dialog(event, d);
}
