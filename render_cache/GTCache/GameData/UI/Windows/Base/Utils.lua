function CalculateListSize(document, element, listName, tableName, bottomPanelName)
    window_el = document: GetElementById('window')
    wnd_content = window_el: GetElementById('PopupContent')
    message_list = wnd_content: GetElementById(listName)
    wnd_height = document.client_height*0.8 - 2*wnd_content.offset_top - message_list.offset_top

    if bottomPanelName and bottomPanelName ~= '' then
    bottom_panel = wnd_content: GetElementById(bottomPanelName)
      
    if bottom_panel then
        wnd_height = wnd_height - bottom_panel.client_height 
    end
    end

    message_list.style.height = wnd_height..'px'
    message_table = wnd_content: GetElementById(tableName)
    message_table.style['height'] =wnd_height..'px'
end

function CheckEndOfTable(event, document, element, query, action, rowNum)
    if (rowNum == 0) then
        return
    end

    first_raw_el = element:QuerySelector(query)
    start_load_pos = element.scroll_height - first_raw_el.client_height - element.client_height

    if (start_load_pos < element.scroll_top) then
        OnEvent(event, action)
    end
end