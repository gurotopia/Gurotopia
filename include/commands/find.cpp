#include "pch.hpp"
#include "find.hpp"

void find(ENetEvent& event, const std::string_view text)
{
    gt_packet(*event.peer, false, 0, {
        "OnDialogRequest", 
        "set_default_color|`o\n"
        "add_text_input|n|Search: ||26|\n"
        "add_searchable_item_list||sourceType:allItems;listType:iconWithCustomLabel;resultLimit:30|n|\n"
        "add_quick_exit|\n"
        "end_dialog|find|||"
    });
}