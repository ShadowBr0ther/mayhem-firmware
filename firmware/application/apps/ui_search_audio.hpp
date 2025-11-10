/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_search.hpp"

#include "audio.hpp"
#include "freqman.hpp"

namespace ui {

class SearchAudioView : public SearchView {
   public:
    SearchAudioView(NavigationView& nav);
    ~SearchAudioView();

    SearchAudioView(const SearchAudioView&) = delete;
    SearchAudioView(SearchAudioView&&) = delete;
    SearchAudioView& operator=(const SearchAudioView&) = delete;
    SearchAudioView& operator=(SearchAudioView&&) = delete;

    std::string title() const override { return "Search Audio"; };

   protected:
    const char* locked_status_text() const override;
    void on_lock_acquired(rf::Frequency frequency, size_t slice_index) override;
    void on_lock_released() override;
    void on_detection_reset() override;
    bool should_hold_locked_slice() const override;
    size_t locked_slice_index() const override;
    rf::Frequency hold_frequency() const override;

   private:
    struct SearchAudioSettings {
        uint8_t modulation = static_cast<uint8_t>(ReceiverModel::Mode::NarrowbandFMAudio);
        uint8_t am_config = 0;
        uint8_t nbfm_config = 0;
        uint8_t wfm_config = 0;
    };

    SearchAudioSettings audio_settings_{};

    ReceiverModel::Mode current_modulation_{ReceiverModel::Mode::NarrowbandFMAudio};
    bool audio_muted_{true};
    uint8_t locked_slice_index_{0};

    Labels labels_audio_{
        {{UI_POS_X(1), UI_POS_Y(4)}, "Mod:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(10), UI_POS_Y(4)}, "BW:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_modulation{
        {UI_POS_X(5), UI_POS_Y(4)},
        4,
        {{"AM", static_cast<int32_t>(ReceiverModel::Mode::AMAudio)},
         {"NFM", static_cast<int32_t>(ReceiverModel::Mode::NarrowbandFMAudio)},
         {"WFM", static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio)}}};

    OptionsField options_bandwidth{
        {UI_POS_X(13), UI_POS_Y(4)},
        7,
        {}};

    void on_modulation_changed(ReceiverModel::Mode new_mode);
    void update_modulation(ReceiverModel::Mode modulation);
    void update_bandwidth_options(ReceiverModel::Mode modulation);
    void apply_bandwidth_selection();
    void update_audio_sample_rate();
};

} /* namespace ui */

