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

#include "ui_search_audio.hpp"

#include "baseband_api.hpp"
#include "binder.hpp"

using namespace portapack;
using std::literals::operator""sv;

namespace ui {

SearchAudioView::SearchAudioView(NavigationView& nav)
    : SearchView(nav,
                 "rx_search_audio"sv,
                 {{"modulation"sv, &audio_settings_.modulation},
                  {"am_config"sv, &audio_settings_.am_config},
                  {"nbfm_config"sv, &audio_settings_.nbfm_config},
                  {"wfm_config"sv, &audio_settings_.wfm_config}}) {
    add_children({&labels_audio_, &options_modulation, &options_bandwidth});

    bind(options_modulation, audio_settings_.modulation);
    options_modulation.set_by_value(audio_settings_.modulation);
    options_modulation.on_change = [this](size_t, OptionsField::value_t value) {
        audio_settings_.modulation = value;
        on_modulation_changed(static_cast<ReceiverModel::Mode>(value));
    };

    on_modulation_changed(static_cast<ReceiverModel::Mode>(audio_settings_.modulation));

    audio::output::start();
    audio::output::mute();
    audio_muted_ = true;
}

SearchAudioView::~SearchAudioView() = default;

const char* SearchAudioView::locked_status_text() const {
    return "Locked (audio)";
}

void SearchAudioView::on_lock_acquired(rf::Frequency frequency, size_t slice_index) {
    SearchView::on_lock_acquired(frequency, slice_index);

    locked_slice_index_ = static_cast<uint8_t>(slice_index);
    receiver_model.set_target_frequency(frequency);

    if (audio_muted_) {
        audio::output::unmute();
        audio_muted_ = false;
    }
}

void SearchAudioView::on_lock_released() {
    SearchView::on_lock_released();

    locked_slice_index_ = 0;
    if (!audio_muted_) {
        audio::output::mute();
        audio_muted_ = true;
    }
}

void SearchAudioView::on_detection_reset() {
    locked_slice_index_ = 0;
    if (!audio_muted_) {
        audio::output::mute();
    }
    audio_muted_ = true;
}

bool SearchAudioView::should_hold_locked_slice() const {
    return is_locked();
}

size_t SearchAudioView::locked_slice_index() const {
    return locked_slice_index_;
}

rf::Frequency SearchAudioView::hold_frequency() const {
    return resolved_frequency;
}

void SearchAudioView::on_modulation_changed(ReceiverModel::Mode new_mode) {
    current_modulation_ = new_mode;
    update_modulation(new_mode);
    update_bandwidth_options(new_mode);
    apply_bandwidth_selection();
}

void SearchAudioView::update_modulation(ReceiverModel::Mode modulation) {
    audio::output::mute();
    audio_muted_ = true;

    baseband::spectrum_streaming_stop();
    baseband::shutdown();

    portapack::spi_flash::image_tag_t image_tag;
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            image_tag = portapack::spi_flash::image_tag_am_audio;
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            image_tag = portapack::spi_flash::image_tag_nfm_audio;
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            image_tag = portapack::spi_flash::image_tag_wfm_audio;
            break;
        default:
            image_tag = portapack::spi_flash::image_tag_nfm_audio;
            break;
    }

    baseband::run_image(image_tag);
    baseband::set_spectrum(SEARCH_SLICE_WIDTH, 31);

    receiver_model.set_modulation(modulation);
    receiver_model.set_sampling_rate(SEARCH_SLICE_WIDTH);
    receiver_model.set_baseband_bandwidth(SEARCH_SLICE_WIDTH / 2);
    receiver_model.enable();

    update_audio_sample_rate();

    baseband::spectrum_streaming_start();
}

void SearchAudioView::update_bandwidth_options(ReceiverModel::Mode modulation) {
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            freqman_set_bandwidth_option(AM_MODULATION, options_bandwidth);
            options_bandwidth.on_change = [this](size_t, OptionsField::value_t value) {
                audio_settings_.am_config = value;
                receiver_model.set_am_configuration(value);
            };
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            freqman_set_bandwidth_option(NFM_MODULATION, options_bandwidth);
            options_bandwidth.on_change = [this](size_t, OptionsField::value_t value) {
                audio_settings_.nbfm_config = value;
                receiver_model.set_nbfm_configuration(value);
            };
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            freqman_set_bandwidth_option(WFM_MODULATION, options_bandwidth);
            options_bandwidth.on_change = [this](size_t, OptionsField::value_t value) {
                audio_settings_.wfm_config = value;
                receiver_model.set_wfm_configuration(value);
            };
            break;
        default:
            options_bandwidth.set_options({});
            options_bandwidth.on_change = nullptr;
            break;
    }
}

void SearchAudioView::apply_bandwidth_selection() {
    switch (current_modulation_) {
        case ReceiverModel::Mode::AMAudio:
            options_bandwidth.set_by_value(audio_settings_.am_config);
            receiver_model.set_am_configuration(audio_settings_.am_config);
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            options_bandwidth.set_by_value(audio_settings_.nbfm_config);
            receiver_model.set_nbfm_configuration(audio_settings_.nbfm_config);
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            options_bandwidth.set_by_value(audio_settings_.wfm_config);
            receiver_model.set_wfm_configuration(audio_settings_.wfm_config);
            break;
        default:
            break;
    }
}

void SearchAudioView::update_audio_sample_rate() {
    using audio::Rate;
    switch (current_modulation_) {
        case ReceiverModel::Mode::AMAudio:
            audio::set_rate(Rate::Hz_12000);
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            audio::set_rate(Rate::Hz_24000);
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            audio::set_rate(Rate::Hz_48000);
            break;
        default:
            audio::set_rate(Rate::Hz_24000);
            break;
    }
}

} /* namespace ui */

