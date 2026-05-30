#include "team/protocol/team_mgmt.h"

#include <cassert>
#include <vector>

namespace
{

team::proto::TeamKeyRequest makeRequest()
{
    team::proto::TeamKeyRequest request{};
    request.team_id[0] = 0x42;
    request.team_id[1] = 0x24;
    request.current_key_id = 7;
    request.requester_id = 0x22222222;
    return request;
}

void testKeyRequestRoundTrips()
{
    const auto request = makeRequest();
    std::vector<uint8_t> payload;
    assert(team::proto::encodeTeamKeyRequest(request, payload));

    team::proto::TeamKeyRequest decoded{};
    assert(team::proto::decodeTeamKeyRequest(payload.data(),
                                             payload.size(),
                                             &decoded));
    assert(decoded.team_id == request.team_id);
    assert(decoded.current_key_id == 7);
    assert(decoded.requester_id == 0x22222222);
}

void testMgmtEnvelopeCarriesKeyRequestType()
{
    std::vector<uint8_t> payload;
    assert(team::proto::encodeTeamKeyRequest(makeRequest(), payload));

    std::vector<uint8_t> wire;
    assert(team::proto::encodeTeamMgmtMessage(
        team::proto::TeamMgmtType::KeyRequest,
        payload,
        wire));

    uint8_t version = 0;
    team::proto::TeamMgmtType type = team::proto::TeamMgmtType::Status;
    std::vector<uint8_t> decoded_payload;
    assert(team::proto::decodeTeamMgmtMessage(wire.data(),
                                              wire.size(),
                                              &version,
                                              &type,
                                              decoded_payload));
    assert(version == team::proto::kTeamMgmtVersion);
    assert(type == team::proto::TeamMgmtType::KeyRequest);
    assert(decoded_payload == payload);
}

void testDecodeRejectsShortPayloads()
{
    uint8_t payload[4] = {0};
    team::proto::TeamKeyRequest decoded{};
    assert(!team::proto::decodeTeamKeyRequest(payload,
                                              sizeof(payload),
                                              &decoded));
}

} // namespace

int main()
{
    testKeyRequestRoundTrips();
    testMgmtEnvelopeCarriesKeyRequestType();
    testDecodeRejectsShortPayloads();
    return 0;
}
