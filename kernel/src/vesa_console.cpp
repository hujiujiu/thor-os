//=======================================================================
// Copyright Baptiste Wicht 2013-2016.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://www.opensource.org/licenses/MIT)
//=======================================================================

#include "vesa_console.hpp"
#include "vesa.hpp"
#include "early_memory.hpp"

namespace {

constexpr const size_t MARGIN = 10;
constexpr const size_t PADDING = 5;
constexpr const size_t LEFT = MARGIN + PADDING;
constexpr const size_t TOP = 40;

size_t _lines;
size_t _columns;
uint32_t _color;
size_t buffer_size;

} //end of anonymous namespace

void vesa_console::init(){
    auto& block = *reinterpret_cast<vesa::mode_info_block_t*>(early::vesa_mode_info_address);

    _columns = (block.width - (MARGIN + PADDING) * 2) / 8;
    _lines = (block.height - TOP - MARGIN - PADDING) / 16;
    _color = vesa::make_color(0, 255, 0);

    buffer_size = block.height * block.bytes_per_scan_line;

    vesa::draw_hline(MARGIN, MARGIN, block.width - 2 * MARGIN, _color);
    vesa::draw_hline(MARGIN, 35, block.width - 2 * MARGIN, _color);
    vesa::draw_hline(MARGIN, block.height - MARGIN, block.width - 2 * MARGIN, _color);

    vesa::draw_vline(MARGIN, MARGIN, block.height - 2 * MARGIN, _color);
    vesa::draw_vline(block.width - MARGIN, MARGIN, block.height - 2 * MARGIN, _color);

    auto title_left = (block.width - 4 * 8) / 2;
    vesa::draw_char(title_left, PADDING + MARGIN, 'T', _color);
    vesa::draw_char(title_left + 8, PADDING + MARGIN, 'H', _color);
    vesa::draw_char(title_left + 16, PADDING + MARGIN, 'O', _color);
    vesa::draw_char(title_left + 24, PADDING + MARGIN, 'R', _color);
}

size_t vesa_console::lines(){
    return _lines;
}

size_t vesa_console::columns(){
    return _columns;
}

void vesa_console::clear(){
    vesa::draw_rect(LEFT, TOP, _columns * 8, _lines* 16, 0, 0, 0);
}

void vesa_console::scroll_up(){
    vesa::move_lines_up(TOP + 16, LEFT, _columns * 8, (_lines - 1) * 16, 16);
    vesa::draw_rect(LEFT, TOP + (_lines - 1) * 16, _columns * 8, 16, 0, 0, 0);
}

void vesa_console::print_char(size_t line, size_t column, char c){
    vesa::draw_char(LEFT + 8 * column, TOP + 16 * line, c, _color);
}

void* vesa_console::save(void* buffer){
    void* buffer32 = static_cast<uint32_t*>(buffer);
    if(!buffer32){
        buffer32 = new uint32_t[buffer_size];
    }

    vesa::save(static_cast<char*>(buffer32));

    return buffer32;
}

void vesa_console::restore(void* buffer){
    vesa::redraw(static_cast<char*>(buffer));
}
