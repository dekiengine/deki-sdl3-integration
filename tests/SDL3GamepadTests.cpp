// SDL3Gamepad against SDL's own virtual gamepad: buttons and sticks become the
// keys a handheld's buttons send.

#include <gtest/gtest.h>

#include "Keys.h"
#include "SDL3Gamepad.h"

#include <SDL3/SDL.h>

#include <vector>

using namespace DekiSdl3;
namespace Keys = DekiInput::Keys;
using DekiInput::InputEvent;
using DekiInput::InputEventType;

namespace
{

class SDL3GamepadTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(SDL_Init(SDL_INIT_EVENTS)) << SDL_GetError();
        ASSERT_TRUE(pads.Initialize()) << SDL_GetError();
        pads.RegisterEventCallback([this](const InputEvent& e) { events.push_back(e); });

        SDL_VirtualJoystickDesc desc;
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        desc.name = "Test pad";
        id = SDL_AttachVirtualJoystick(&desc);
        ASSERT_NE(id, 0u) << SDL_GetError();
        stick = SDL_OpenJoystick(id);
        ASSERT_NE(stick, nullptr) << SDL_GetError();
        pads.Update();  // picks the pad up
        ASSERT_EQ(pads.PadCount(), 1u);
        events.clear();
    }

    void TearDown() override
    {
        pads.Shutdown();
        if (stick)
        {
            SDL_CloseJoystick(stick);
        }
        SDL_DetachVirtualJoystick(id);
        SDL_Quit();
    }

    void Button(SDL_GamepadButton button, bool down)
    {
        ASSERT_TRUE(SDL_SetJoystickVirtualButton(stick, button, down));
        SDL_UpdateJoysticks();
        pads.Update();
    }

    void Axis(SDL_GamepadAxis axis, Sint16 value)
    {
        ASSERT_TRUE(SDL_SetJoystickVirtualAxis(stick, axis, value));
        SDL_UpdateJoysticks();
        pads.Update();
    }

    SDL3Gamepad pads;
    SDL_JoystickID id = 0;
    SDL_Joystick* stick = nullptr;
    std::vector<InputEvent> events;
};

}  // namespace

TEST_F(SDL3GamepadTest, TheDPadIsTheArrowsHeldAsLongAsTheButton)
{
    Button(SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, InputEventType::KeyDown);
    EXPECT_EQ(events[0].key, Keys::Up);
    pads.Update();
    EXPECT_TRUE(pads.IsKeyPressed(Keys::Up)) << "still held";
    Button(SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[1].type, InputEventType::KeyUp);
    EXPECT_FALSE(pads.IsKeyPressed(Keys::Up));
}

TEST_F(SDL3GamepadTest, FaceButtonsAreTheHandheldKeys)
{
    Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
    EXPECT_TRUE(pads.IsKeyPressed(Keys::Enter));
    Button(SDL_GAMEPAD_BUTTON_SOUTH, false);
    Button(SDL_GAMEPAD_BUTTON_WEST, true);
    EXPECT_TRUE(pads.IsKeyPressed(Keys::Space));
    Button(SDL_GAMEPAD_BUTTON_EAST, true);
    EXPECT_TRUE(pads.IsKeyPressed(Keys::Esc));
}

TEST_F(SDL3GamepadTest, TheLeftStickPastHalfIsAnArrowAndATriggerAKey)
{
    Axis(SDL_GAMEPAD_AXIS_LEFTX, 8000);
    EXPECT_FALSE(pads.IsKeyPressed(Keys::Right)) << "a light touch is not a press";
    Axis(SDL_GAMEPAD_AXIS_LEFTX, 30000);
    EXPECT_TRUE(pads.IsKeyPressed(Keys::Right));
    Axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 30000);
    EXPECT_TRUE(pads.IsKeyPressed(Keys::ForCharacter('c')));
}

TEST_F(SDL3GamepadTest, APadPluggedInLaterIsPickedUpOnTheNextUpdate)
{
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = "Second pad";
    const SDL_JoystickID second = SDL_AttachVirtualJoystick(&desc);
    ASSERT_NE(second, 0u) << SDL_GetError();
    pads.Update();
    EXPECT_EQ(pads.PadCount(), 2u);
    SDL_DetachVirtualJoystick(second);
    pads.Update();
    EXPECT_EQ(pads.PadCount(), 1u) << "an unplugged pad goes at once";
}
