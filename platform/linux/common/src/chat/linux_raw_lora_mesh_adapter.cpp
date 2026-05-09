#include "chat/linux_raw_lora_mesh_adapter.h"

#include <algorithm>
#include <chrono>
#include <cstring>

#include "platform/linux/runtime_packet_log.h"

namespace trailmate::linux_app
{
namespace
{

constexpr std::uint8_t kMagic0 = 'T';
constexpr std::uint8_t kMagic1 = 'M';
constexpr std::uint8_t kMagic2 = 'L';
constexpr std::uint8_t kMagic3 = '1';
constexpr std::uint8_t kVersion = 1;
constexpr std::uint8_t kBroadcastFlags = 0x01;
constexpr std::size_t kHeaderSize = 26;
constexpr std::size_t kMaxPayloadSize = 190;

std::uint32_t now_seconds()
{
    using clock = std::chrono::system_clock;
    return static_cast<std::uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            clock::now().time_since_epoch())
            .count());
}

void write_u16(std::uint8_t* out, std::uint16_t value)
{
    out[0] = static_cast<std::uint8_t>(value & 0xFF);
    out[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
}

void write_u32(std::uint8_t* out, std::uint32_t value)
{
    out[0] = static_cast<std::uint8_t>(value & 0xFF);
    out[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    out[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
    out[3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
}

std::uint16_t read_u16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(data[0]) |
           (static_cast<std::uint16_t>(data[1]) << 8);
}

std::uint32_t read_u32(const std::uint8_t* data)
{
    return static_cast<std::uint32_t>(data[0]) |
           (static_cast<std::uint32_t>(data[1]) << 8) |
           (static_cast<std::uint32_t>(data[2]) << 16) |
           (static_cast<std::uint32_t>(data[3]) << 24);
}

::platform::linux_runtime::PacketLogEntry make_lora_log(
    ::platform::linux_runtime::PacketLogDirection direction,
    const std::uint8_t* frame,
    std::size_t frame_size,
    const char* title,
    const char* summary)
{
    ::platform::linux_runtime::PacketLogEntry entry{};
    entry.source = ::platform::linux_runtime::PacketLogSource::Lora;
    entry.direction = direction;
    entry.title = title == nullptr ? "LoRa frame" : title;
    entry.summary = summary == nullptr ? "" : summary;
    entry.raw_hex = ::platform::linux_runtime::hex_bytes(frame, frame_size);
    const std::size_t header_len = std::min<std::size_t>(kHeaderSize, frame_size);
    if (header_len > 0)
    {
        entry.segments.push_back({
            .kind = ::platform::linux_runtime::PacketLogSegmentKind::Header,
            .label = "head",
            .text = ::platform::linux_runtime::hex_bytes(frame, header_len),
        });
    }
    if (frame_size > kHeaderSize)
    {
        entry.segments.push_back({
            .kind = ::platform::linux_runtime::PacketLogSegmentKind::Body,
            .label = "body",
            .text = ::platform::linux_runtime::hex_bytes(
                frame + kHeaderSize, frame_size - kHeaderSize),
        });
    }
    entry.segments.push_back({
        .kind = ::platform::linux_runtime::PacketLogSegmentKind::Meta,
        .label = "crc",
        .text = "radio",
    });
    return entry;
}

::platform::linux_runtime::Sx126xLoRaConfig derive_radio_config(
    const ::chat::MeshConfig& config)
{
    auto out = ::platform::linux_runtime::Sx126xRadio::
        defaultLoRaConfigFromEnvironment();
    if (config.override_frequency_mhz > 0.0f)
    {
        out.freq_mhz = config.override_frequency_mhz;
    }
    else if (config.meshcore_freq_mhz > 0.0f &&
             config.meshcore_freq_mhz < 1000.0f)
    {
        out.freq_mhz = config.meshcore_freq_mhz;
    }
    out.freq_mhz += config.frequency_offset_mhz;
    if (config.bandwidth_khz > 0.0f)
    {
        out.bw_khz = config.bandwidth_khz;
    }
    if (config.spread_factor >= 5 && config.spread_factor <= 12)
    {
        out.sf = config.spread_factor;
    }
    if (config.coding_rate >= 5 && config.coding_rate <= 8)
    {
        out.cr = config.coding_rate;
    }
    if (config.tx_power != 0)
    {
        out.tx_power_dbm = std::clamp<std::int8_t>(config.tx_power, -9, 22);
    }
    return out;
}

} // namespace

LinuxRawLoraMeshAdapter::LinuxRawLoraMeshAdapter(::chat::NodeId self_node_id)
    : radio_(::platform::linux_runtime::Sx126xRadio::instance()),
      self_node_id_(self_node_id)
{
}

bool LinuxRawLoraMeshAdapter::hardwareCandidatePresent()
{
    return ::platform::linux_runtime::Sx126xRadio::hardwareCandidatePresent();
}

bool LinuxRawLoraMeshAdapter::begin()
{
    return ensureRadioReady();
}

void LinuxRawLoraMeshAdapter::tick()
{
    if (!ensureRadioReady())
    {
        return;
    }
    for (int i = 0; i < 4; ++i)
    {
        ::platform::linux_runtime::Sx126xPacket packet{};
        if (!radio_.pollReceive(&packet))
        {
            break;
        }
        ::platform::linux_runtime::append_packet_log(
            make_lora_log(::platform::linux_runtime::PacketLogDirection::Rx,
                          packet.data.data(),
                          packet.size,
                          "LoRa raw RX",
                          "SX1262 packet received."));
        raw_packets_.push_back(packet);
        (void)parseFrame(packet);
    }
}

bool LinuxRawLoraMeshAdapter::takePendingSendResult(::chat::MessageId& out_msg_id,
                                                    bool& out_ok)
{
    if (pending_results_.empty())
    {
        return false;
    }
    const PendingResult result = pending_results_.front();
    pending_results_.pop_front();
    out_msg_id = result.msg_id;
    out_ok = result.ok;
    return true;
}

::chat::MeshCapabilities LinuxRawLoraMeshAdapter::getCapabilities() const
{
    return {
        .supports_unicast_text = true,
        .supports_unicast_appdata = true,
        .supports_broadcast_appdata = true,
        .supports_appdata_ack = false,
        .provides_appdata_sender = true,
        .supports_node_info = false,
        .supports_pki = false,
        .supports_discovery_actions = false,
    };
}

bool LinuxRawLoraMeshAdapter::sendText(::chat::ChannelId channel,
                                       const std::string& text,
                                       ::chat::MessageId* out_msg_id,
                                       ::chat::NodeId peer)
{
    return sendTextWithId(channel, text, 0, out_msg_id, peer);
}

bool LinuxRawLoraMeshAdapter::sendTextWithId(::chat::ChannelId channel,
                                             const std::string& text,
                                             ::chat::MessageId forced_msg_id,
                                             ::chat::MessageId* out_msg_id,
                                             ::chat::NodeId peer)
{
    if (text.empty())
    {
        return false;
    }
    const ::chat::MessageId msg_id =
        forced_msg_id != 0 ? forced_msg_id : nextMessageId();
    if (out_msg_id != nullptr)
    {
        *out_msg_id = msg_id;
    }

    const auto* bytes =
        reinterpret_cast<const std::uint8_t*>(text.data());
    const bool ok = sendFrame(PacketKind::Text,
                              channel,
                              peer == 0 ? 0xFFFFFFFFUL : peer,
                              msg_id,
                              0,
                              bytes,
                              text.size());
    if (msg_id != 0)
    {
        pending_results_.push_back({.msg_id = msg_id, .ok = ok});
    }
    return ok;
}

bool LinuxRawLoraMeshAdapter::pollIncomingText(::chat::MeshIncomingText* out)
{
    tick();
    if (incoming_text_.empty())
    {
        return false;
    }
    if (out != nullptr)
    {
        *out = incoming_text_.front();
    }
    incoming_text_.pop_front();
    return true;
}

bool LinuxRawLoraMeshAdapter::sendAppData(::chat::ChannelId channel,
                                          std::uint32_t portnum,
                                          const std::uint8_t* payload,
                                          std::size_t len,
                                          ::chat::NodeId dest,
                                          bool want_ack,
                                          ::chat::MessageId packet_id,
                                          bool want_response)
{
    (void)want_ack;
    (void)want_response;
    if (payload == nullptr || len == 0)
    {
        return false;
    }
    return sendFrame(PacketKind::AppData,
                     channel,
                     dest == 0 ? 0xFFFFFFFFUL : dest,
                     packet_id != 0 ? packet_id : nextMessageId(),
                     portnum,
                     payload,
                     len);
}

bool LinuxRawLoraMeshAdapter::pollIncomingData(::chat::MeshIncomingData* out)
{
    tick();
    if (incoming_data_.empty())
    {
        return false;
    }
    if (out != nullptr)
    {
        *out = incoming_data_.front();
    }
    incoming_data_.pop_front();
    return true;
}

void LinuxRawLoraMeshAdapter::applyConfig(const ::chat::MeshConfig& config)
{
    config_ = config;
    tx_enabled_ = config.tx_enabled;
    if (started_)
    {
        (void)radio_.configureLoRa(derive_radio_config(config_));
    }
}

void LinuxRawLoraMeshAdapter::setUserInfo(const char* long_name,
                                          const char* short_name)
{
    long_name_ = long_name == nullptr ? "" : long_name;
    short_name_ = short_name == nullptr ? "" : short_name;
}

::chat::NodeId LinuxRawLoraMeshAdapter::getNodeId() const
{
    return self_node_id_;
}

bool LinuxRawLoraMeshAdapter::isReady() const
{
    return started_ && radio_.isOnline();
}

bool LinuxRawLoraMeshAdapter::pollIncomingRawPacket(std::uint8_t* out_data,
                                                    std::size_t& out_len,
                                                    std::size_t max_len)
{
    tick();
    if (raw_packets_.empty())
    {
        return false;
    }
    const auto packet = raw_packets_.front();
    raw_packets_.pop_front();
    out_len = std::min(max_len, packet.size);
    if (out_data != nullptr && out_len > 0)
    {
        std::memcpy(out_data, packet.data.data(), out_len);
    }
    return true;
}

void LinuxRawLoraMeshAdapter::processSendQueue()
{
    tick();
}

void LinuxRawLoraMeshAdapter::setSelfNodeId(::chat::NodeId id)
{
    self_node_id_ = id;
}

std::string LinuxRawLoraMeshAdapter::statusText() const
{
    if (started_ && radio_.isOnline())
    {
        return "SX1262 raw LoRa transport ready.";
    }
    return last_status_;
}

bool LinuxRawLoraMeshAdapter::ensureRadioReady()
{
    if (started_ && radio_.isOnline())
    {
        return true;
    }
    if (!hardwareCandidatePresent())
    {
        last_status_ = "No Linux LoRa SPI endpoint is present.";
        return false;
    }
    if (!radio_.acquire())
    {
        last_status_ = std::string("SX1262 acquire failed: ") +
                       radio_.lastError();
        return false;
    }
    if (!radio_.configureLoRa(derive_radio_config(config_)))
    {
        last_status_ = std::string("SX1262 LoRa config failed: ") +
                       radio_.lastError();
        return false;
    }
    started_ = true;
    last_status_ = "SX1262 raw LoRa transport ready.";
    return true;
}

::chat::MessageId LinuxRawLoraMeshAdapter::nextMessageId()
{
    ++next_msg_id_;
    if (next_msg_id_ == 0)
    {
        next_msg_id_ = 1;
    }
    return next_msg_id_;
}

bool LinuxRawLoraMeshAdapter::sendFrame(PacketKind kind,
                                        ::chat::ChannelId channel,
                                        ::chat::NodeId dest,
                                        ::chat::MessageId msg_id,
                                        std::uint32_t portnum,
                                        const std::uint8_t* payload,
                                        std::size_t len)
{
    if (!tx_enabled_ || payload == nullptr || len == 0 ||
        len > kMaxPayloadSize || !ensureRadioReady())
    {
        return false;
    }

    std::uint8_t frame[255] = {};
    frame[0] = kMagic0;
    frame[1] = kMagic1;
    frame[2] = kMagic2;
    frame[3] = kMagic3;
    frame[4] = kVersion;
    frame[5] = static_cast<std::uint8_t>(kind);
    frame[6] = static_cast<std::uint8_t>(channel);
    frame[7] = dest == 0xFFFFFFFFUL ? kBroadcastFlags : 0;
    write_u32(frame + 8, self_node_id_);
    write_u32(frame + 12, dest);
    write_u32(frame + 16, msg_id);
    write_u32(frame + 20, portnum);
    write_u16(frame + 24, static_cast<std::uint16_t>(len));
    std::memcpy(frame + kHeaderSize, payload, len);
    const std::size_t frame_size = kHeaderSize + len;
    ::platform::linux_runtime::append_packet_log(
        make_lora_log(::platform::linux_runtime::PacketLogDirection::Tx,
                      frame,
                      frame_size,
                      kind == PacketKind::Text ? "LoRa text TX"
                                               : "LoRa appdata TX",
                      "Trail Mate raw LoRa frame queued."));
    return radio_.transmit(frame, frame_size);
}

bool LinuxRawLoraMeshAdapter::parseFrame(
    const ::platform::linux_runtime::Sx126xPacket& packet)
{
    if (packet.size < kHeaderSize)
    {
        return false;
    }
    const auto* data = packet.data.data();
    if (data[0] != kMagic0 || data[1] != kMagic1 || data[2] != kMagic2 ||
        data[3] != kMagic3 || data[4] != kVersion)
    {
        return false;
    }

    const auto kind = static_cast<PacketKind>(data[5]);
    const auto channel = static_cast<::chat::ChannelId>(data[6]);
    const ::chat::NodeId from = read_u32(data + 8);
    const ::chat::NodeId to = read_u32(data + 12);
    const ::chat::MessageId msg_id = read_u32(data + 16);
    const std::uint32_t portnum = read_u32(data + 20);
    const std::uint16_t payload_len = read_u16(data + 24);
    if (payload_len > packet.size - kHeaderSize)
    {
        return false;
    }
    if (from == self_node_id_ ||
        (to != 0xFFFFFFFFUL && to != 0 && to != self_node_id_))
    {
        return false;
    }

    ::chat::RxMeta meta{};
    meta.origin = ::chat::RxOrigin::Mesh;
    meta.rx_timestamp_s = now_seconds();
    meta.time_source = ::chat::RxTimeSource::DeviceUtc;
    meta.direct = to == self_node_id_;
    meta.rssi_dbm_x10 = static_cast<std::int16_t>(packet.rssi_dbm * 10.0f);
    meta.snr_db_x10 = static_cast<std::int16_t>(packet.snr_db * 10.0f);
    meta.freq_hz = packet.freq_hz;
    meta.bw_hz = packet.bw_hz;
    meta.sf = packet.sf;
    meta.cr = packet.cr;

    const auto* payload = data + kHeaderSize;
    if (kind == PacketKind::Text)
    {
        ::chat::MeshIncomingText text{};
        text.channel = channel;
        text.from = from;
        text.to = to;
        text.msg_id = msg_id;
        text.timestamp = meta.rx_timestamp_s;
        text.text.assign(reinterpret_cast<const char*>(payload), payload_len);
        text.hop_limit = 0;
        text.encrypted = false;
        text.rx_meta = meta;
        incoming_text_.push_back(std::move(text));
        return true;
    }
    if (kind == PacketKind::AppData)
    {
        ::chat::MeshIncomingData app{};
        app.portnum = portnum;
        app.from = from;
        app.to = to;
        app.packet_id = msg_id;
        app.channel = channel;
        app.hop_limit = 0;
        app.payload.assign(payload, payload + payload_len);
        app.rx_meta = meta;
        incoming_data_.push_back(std::move(app));
        return true;
    }
    return false;
}

} // namespace trailmate::linux_app
