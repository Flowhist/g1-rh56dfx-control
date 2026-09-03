#include "rh56/hand_api.hpp"

#include <iostream>

int main()
{
    rh56::SetTargetsRequest source;
    source.hand = "right";
    source.joint_mask = 4;
    source.q = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
    source.command_id = 77;
    source.timeout_ms = 500;

    rh56::SetTargetsRequest decoded;
    unitree::common::FromJsonString(
        unitree::common::ToJsonString(source), decoded);
    if (decoded.hand != source.hand || decoded.joint_mask != source.joint_mask ||
        decoded.q != source.q || decoded.command_id != source.command_id ||
        decoded.timeout_ms != source.timeout_ms) {
        std::cerr << "SetTargetsRequest JSON round trip failed\n";
        return 1;
    }

    rh56::StateReply state;
    state.hand = "left";
    state.online = true;
    state.feedback_q = source.q;
    state.error = {0, 0, 0, 0, 0, 0};
    state.status = {1, 1, 1, 1, 1, 1};
    state.temperature = {30, 31, 32, 33, 34, 35};
    state.force_limit = {200, 200, 200, 200, 200, 200};
    state.current_limit = {150, 150, 150, 150, 150, 150};
    rh56::StateReply decoded_state;
    unitree::common::FromJsonString(
        unitree::common::ToJsonString(state), decoded_state);
    if (!decoded_state.online || decoded_state.hand != "left" ||
        decoded_state.feedback_q != source.q ||
        decoded_state.temperature != state.temperature ||
        decoded_state.force_limit != state.force_limit ||
        decoded_state.current_limit != state.current_limit) {
        std::cerr << "StateReply JSON round trip failed\n";
        return 1;
    }

    rh56::GetStateRequest state_request;
    state_request.hand = "right";
    state_request.refresh = true;
    rh56::GetStateRequest decoded_state_request;
    unitree::common::FromJsonString(
        unitree::common::ToJsonString(state_request), decoded_state_request);
    if (!decoded_state_request.refresh || decoded_state_request.hand != "right") {
        std::cerr << "GetStateRequest JSON round trip failed\n";
        return 1;
    }

    rh56::HoldRequest hold;
    hold.hand = "left";
    hold.request_id = 79;
    rh56::HoldRequest decoded_hold;
    unitree::common::FromJsonString(
        unitree::common::ToJsonString(hold), decoded_hold);
    if (decoded_hold.hand != hold.hand || decoded_hold.request_id != 79) {
        std::cerr << "HoldRequest JSON round trip failed\n";
        return 1;
    }

    rh56::GripRequest grip;
    grip.hand = "right";
    grip.q = source.q;
    grip.force_grams = 250;
    grip.current_ma = 180;
    grip.command_id = 78;
    grip.timeout_ms = 1000;
    rh56::GripRequest decoded_grip;
    unitree::common::FromJsonString(
        unitree::common::ToJsonString(grip), decoded_grip);
    if (decoded_grip.hand != grip.hand || decoded_grip.q != grip.q ||
        decoded_grip.force_grams != grip.force_grams ||
        decoded_grip.current_ma != grip.current_ma ||
        decoded_grip.command_id != grip.command_id ||
        decoded_grip.timeout_ms != grip.timeout_ms) {
        std::cerr << "GripRequest JSON round trip failed\n";
        return 1;
    }

    std::cout << "hand_api_test: PASS\n";
    return 0;
}
