#pragma once

#include <cstdint>
#include <string>
#include <unitree/common/json/jsonize.hpp>
#include <vector>

namespace rh56 {

inline const std::string kServiceName = "rh56_hand";
inline const std::string kApiVersion = "1.1.0.0";
constexpr int32_t kApiSetTargets = 1001;
constexpr int32_t kApiClearFault = 1002;
constexpr int32_t kApiGetState = 1003;
constexpr int32_t kApiApplyGrip = 1004;
constexpr int32_t kApiHold = 1005;

class SetTargetsRequest : public unitree::common::Jsonize
{
public:
    std::string hand;
    uint32_t joint_mask{0};
    std::vector<float> q;
    uint64_t command_id{0};
    uint32_t timeout_ms{300};

    void fromJson(unitree::common::JsonMap& json) override
    {
        JN_FROM(json, "hand", hand);
        JN_FROM(json, "joint_mask", joint_mask);
        JN_FROM(json, "q", q);
        JN_FROM(json, "command_id", command_id);
        JN_FROM_WEAK(json, "timeout_ms", timeout_ms);
    }

    void toJson(unitree::common::JsonMap& json) const override
    {
        JN_TO(json, "hand", hand);
        JN_TO(json, "joint_mask", joint_mask);
        JN_TO(json, "q", q);
        JN_TO(json, "command_id", command_id);
        JN_TO(json, "timeout_ms", timeout_ms);
    }
};

class ClearFaultRequest : public unitree::common::Jsonize
{
public:
    std::string hand;
    uint32_t joint_mask{0};
    uint64_t request_id{0};

    void fromJson(unitree::common::JsonMap& json) override
    {
        JN_FROM(json, "hand", hand);
        JN_FROM(json, "joint_mask", joint_mask);
        JN_FROM(json, "request_id", request_id);
    }

    void toJson(unitree::common::JsonMap& json) const override
    {
        JN_TO(json, "hand", hand);
        JN_TO(json, "joint_mask", joint_mask);
        JN_TO(json, "request_id", request_id);
    }
};

class GripRequest : public unitree::common::Jsonize
{
public:
    std::string hand;
    std::vector<float> q;
    int32_t force_grams{0};
    int32_t current_ma{0};
    uint64_t command_id{0};
    uint32_t timeout_ms{300};

    void fromJson(unitree::common::JsonMap& json) override
    {
        JN_FROM(json, "hand", hand);
        JN_FROM(json, "q", q);
        JN_FROM(json, "force_grams", force_grams);
        JN_FROM(json, "current_ma", current_ma);
        JN_FROM(json, "command_id", command_id);
        JN_FROM_WEAK(json, "timeout_ms", timeout_ms);
    }

    void toJson(unitree::common::JsonMap& json) const override
    {
        JN_TO(json, "hand", hand);
        JN_TO(json, "q", q);
        JN_TO(json, "force_grams", force_grams);
        JN_TO(json, "current_ma", current_ma);
        JN_TO(json, "command_id", command_id);
        JN_TO(json, "timeout_ms", timeout_ms);
    }
};

class GetStateRequest : public unitree::common::Jsonize
{
public:
    std::string hand;
    bool refresh{false};

    void fromJson(unitree::common::JsonMap& json) override
    {
        JN_FROM(json, "hand", hand);
        JN_FROM_WEAK(json, "refresh", refresh);
    }

    void toJson(unitree::common::JsonMap& json) const override
    {
        JN_TO(json, "hand", hand);
        JN_TO(json, "refresh", refresh);
    }
};

class HoldRequest : public unitree::common::Jsonize
{
public:
    std::string hand;
    uint64_t request_id{0};

    void fromJson(unitree::common::JsonMap& json) override
    {
        JN_FROM(json, "hand", hand);
        JN_FROM(json, "request_id", request_id);
    }

    void toJson(unitree::common::JsonMap& json) const override
    {
        JN_TO(json, "hand", hand);
        JN_TO(json, "request_id", request_id);
    }
};

class OperationReply : public unitree::common::Jsonize
{
public:
    int32_t code{0};
    std::string message;
    uint64_t request_id{0};
    uint32_t affected_mask{0};
    std::vector<int32_t> error;
    std::vector<int32_t> status;
    std::vector<int32_t> temperature;

    void fromJson(unitree::common::JsonMap& json) override
    {
        JN_FROM(json, "code", code);
        JN_FROM(json, "message", message);
        JN_FROM(json, "request_id", request_id);
        JN_FROM(json, "affected_mask", affected_mask);
        JN_FROM(json, "error", error);
        JN_FROM(json, "status", status);
        JN_FROM(json, "temperature", temperature);
    }

    void toJson(unitree::common::JsonMap& json) const override
    {
        JN_TO(json, "code", code);
        JN_TO(json, "message", message);
        JN_TO(json, "request_id", request_id);
        JN_TO(json, "affected_mask", affected_mask);
        JN_TO(json, "error", error);
        JN_TO(json, "status", status);
        JN_TO(json, "temperature", temperature);
    }
};

class StateReply : public unitree::common::Jsonize
{
public:
    int32_t code{0};
    std::string message;
    std::string hand;
    bool online{false};
    std::vector<float> feedback_q;
    std::vector<float> target_q;
    std::vector<int32_t> force;
    std::vector<int32_t> current;
    std::vector<int32_t> force_limit;
    std::vector<int32_t> current_limit;
    std::vector<int32_t> error;
    std::vector<int32_t> status;
    std::vector<int32_t> temperature;
    uint32_t lost_count{0};
    uint64_t last_command_id{0};
    uint64_t timestamp_ms{0};

    void fromJson(unitree::common::JsonMap& json) override
    {
        JN_FROM(json, "code", code);
        JN_FROM(json, "message", message);
        JN_FROM(json, "hand", hand);
        JN_FROM(json, "online", online);
        JN_FROM(json, "feedback_q", feedback_q);
        JN_FROM(json, "target_q", target_q);
        JN_FROM(json, "force", force);
        JN_FROM(json, "current", current);
        JN_FROM(json, "force_limit", force_limit);
        JN_FROM(json, "current_limit", current_limit);
        JN_FROM(json, "error", error);
        JN_FROM(json, "status", status);
        JN_FROM(json, "temperature", temperature);
        JN_FROM(json, "lost_count", lost_count);
        JN_FROM(json, "last_command_id", last_command_id);
        JN_FROM(json, "timestamp_ms", timestamp_ms);
    }

    void toJson(unitree::common::JsonMap& json) const override
    {
        JN_TO(json, "code", code);
        JN_TO(json, "message", message);
        JN_TO(json, "hand", hand);
        JN_TO(json, "online", online);
        JN_TO(json, "feedback_q", feedback_q);
        JN_TO(json, "target_q", target_q);
        JN_TO(json, "force", force);
        JN_TO(json, "current", current);
        JN_TO(json, "force_limit", force_limit);
        JN_TO(json, "current_limit", current_limit);
        JN_TO(json, "error", error);
        JN_TO(json, "status", status);
        JN_TO(json, "temperature", temperature);
        JN_TO(json, "lost_count", lost_count);
        JN_TO(json, "last_command_id", last_command_id);
        JN_TO(json, "timestamp_ms", timestamp_ms);
    }
};

}  // namespace rh56
