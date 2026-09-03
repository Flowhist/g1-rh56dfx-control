#pragma once

#include "hand_api.hpp"
#include "hand_types.hpp"

#include <unitree/robot/client/client.hpp>

namespace rh56 {

class HandClient : public unitree::robot::Client
{
public:
    HandClient() : Client(kServiceName, false) {}

    void Init() override
    {
        SetApiVersion(kApiVersion);
        UT_ROBOT_CLIENT_REG_API_NO_PROI(kApiSetTargets);
        UT_ROBOT_CLIENT_REG_API_NO_PROI(kApiClearFault);
        UT_ROBOT_CLIENT_REG_API_NO_PROI(kApiGetState);
        UT_ROBOT_CLIENT_REG_API_NO_PROI(kApiApplyGrip);
        UT_ROBOT_CLIENT_REG_API_NO_PROI(kApiHold);
    }

    int32_t SetTargets(const SetTargetsRequest& request,
                       OperationReply& reply)
    {
        return CallAndDecode(kApiSetTargets, request, reply);
    }

    int32_t ClearFault(const ClearFaultRequest& request,
                       OperationReply& reply)
    {
        return CallAndDecode(kApiClearFault, request, reply);
    }

    int32_t GetState(const std::string& hand, StateReply& reply,
                     bool refresh = false)
    {
        GetStateRequest request;
        request.hand = hand;
        request.refresh = refresh;
        return CallAndDecode(kApiGetState, request, reply);
    }

    int32_t ApplyGrip(const GripRequest& request, OperationReply& reply)
    {
        return CallAndDecode(kApiApplyGrip, request, reply);
    }

    int32_t Hold(const HoldRequest& request, OperationReply& reply)
    {
        return CallAndDecode(kApiHold, request, reply);
    }

private:
    template <typename Request, typename Reply>
    int32_t CallAndDecode(int32_t api_id, const Request& request, Reply& reply)
    {
        std::string data;
        const int32_t transport_code =
            Call(api_id, unitree::common::ToJsonString(request), data);
        if (transport_code != 0)
            return transport_code;
        try {
            unitree::common::FromJsonString(data, reply);
            return reply.code;
        } catch (...) {
            return static_cast<int32_t>(ResultCode::kInvalidResponse);
        }
    }
};

}  // namespace rh56
