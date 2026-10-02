#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/create_dialog.hpp"
#include "database/world.hpp"
#include "nuke.hpp"

/* /nuke -> asks for confirmation, the work happens in nuke_confirm */
void nuke(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    ::create_dialog dialog;
    send_varlist(event.peer, {
        "OnDialogRequest",
        dialog
            .set_default_color("`o")
            .add_label_with_icon("big", "`4Nuke World``", 1490 /* nuke-ish icon */)
            .add_spacer("small")
            .add_textbox(std::format("This will remove `4every block`` in `w{}``.", world->name))
            .add_textbox("The main door and its bedrock will be kept.")
            .add_spacer("small")
            .add_smalltext("`4This cannot be undone.``")
            .add_quick_exit()
            .end_dialog("nuke_confirm", "Cancel", "Nuke it")
    });
}
