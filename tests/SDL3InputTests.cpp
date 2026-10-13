// SDL3Input turns SDL's events into the engine's: keys mapped to the codes
// in deki-input's Keys.h, the mouse to pointer events, and the window closing
// to a quit. The events go through SDL's own queue (SDL_PushEvent, then
// Update), with only the event subsystem started, so no window is needed.
// Without a window, pointer coordinates are not scaled to the framebuffer.

#include <gtest/gtest.h>

#include "SDL3Input.h"
#include "Keys.h"

#include <SDL3/SDL.h>

#include <vector>

using namespace DekiSdl3;
namespace Keys = DekiInput::Keys;
using DekiInput::InputEvent;
using DekiInput::InputEventType;

namespace
{

class SDL3InputTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite() { ASSERT_TRUE(SDL_Init(SDL_INIT_EVENTS)) << SDL_GetError(); }
    static void TearDownTestSuite() { SDL_Quit(); }

    void SetUp() override
    {
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
        ASSERT_TRUE(input.Initialize());
        input.RegisterEventCallback([this](const InputEvent& e) { events.push_back(e); });
    }

    void Key(SDL_EventType type, SDL_Keycode key)
    {
        SDL_Event e{};
        e.type = type;
        e.key.key = key;
        e.key.down = (type == SDL_EVENT_KEY_DOWN);
        SDL_PushEvent(&e);
    }

    void Mouse(SDL_EventType type, float x, float y)
    {
        SDL_Event e{};
        e.type = type;
        if (type == SDL_EVENT_MOUSE_MOTION)
        {
            e.motion.x = x;
            e.motion.y = y;
        }
        else
        {
            e.button.x = x;
            e.button.y = y;
            e.button.button = SDL_BUTTON_LEFT;
            e.button.down = (type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        }
        SDL_PushEvent(&e);
    }

    SDL3Input input;
    std::vector<InputEvent> events;
};

}  // namespace

TEST_F(SDL3InputTest, SpecialKeysMapToTheEnginesCodes)
{
    const std::pair<SDL_Keycode, uint32_t> map[] = {
        { SDLK_RETURN, Keys::Enter },   { SDLK_ESCAPE, Keys::Esc }, { SDLK_BACKSPACE, Keys::Backspace },
        { SDLK_TAB, Keys::Tab },        { SDLK_DELETE, Keys::Delete }, { SDLK_UP, Keys::Up },
        { SDLK_DOWN, Keys::Down },      { SDLK_LEFT, Keys::Left },  { SDLK_RIGHT, Keys::Right },
    };
    for (const auto& [sdl, deki] : map)
    {
        events.clear();
        Key(SDL_EVENT_KEY_DOWN, sdl);
        input.Update();
        ASSERT_EQ(events.size(), 1u) << sdl;
        EXPECT_EQ(events[0].type, InputEventType::KeyDown);
        EXPECT_EQ(events[0].key, deki) << sdl;
    }
}

TEST_F(SDL3InputTest, PrintableKeysKeepTheirCharacter)
{
    Key(SDL_EVENT_KEY_DOWN, SDLK_A);
    Key(SDL_EVENT_KEY_DOWN, SDLK_5);
    Key(SDL_EVENT_KEY_DOWN, SDLK_SPACE);
    input.Update();
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].key, uint32_t('a'));
    EXPECT_EQ(events[1].key, Keys::Num5);
    EXPECT_EQ(events[2].key, Keys::Space);
}

TEST_F(SDL3InputTest, KeyStateFollowsDownAndUp)
{
    Key(SDL_EVENT_KEY_DOWN, SDLK_LEFT);
    input.Update();
    EXPECT_TRUE(input.IsKeyPressed(Keys::Left));
    Key(SDL_EVENT_KEY_UP, SDLK_LEFT);
    input.Update();
    EXPECT_FALSE(input.IsKeyPressed(Keys::Left));
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[1].type, InputEventType::KeyUp);
    EXPECT_FALSE(events[1].pressed);
}

TEST_F(SDL3InputTest, KeysWithoutACodeAreNotReported)
{
    // F1 has no engine code. Reported as 0, it read as "key 0 pressed".
    Key(SDL_EVENT_KEY_DOWN, SDLK_F1);
    input.Update();
    EXPECT_TRUE(events.empty());
    EXPECT_FALSE(input.IsKeyPressed(0));
}

TEST_F(SDL3InputTest, TheMouseBecomesPointerEvents)
{
    Mouse(SDL_EVENT_MOUSE_MOTION, 10.0f, 20.0f);
    Mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, 11.0f, 21.0f);
    Mouse(SDL_EVENT_MOUSE_BUTTON_UP, 12.0f, 22.0f);
    input.Update();
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].type, InputEventType::MouseMove);
    EXPECT_EQ(events[0].x, 10);
    EXPECT_EQ(events[0].y, 20);
    EXPECT_EQ(events[1].type, InputEventType::MouseButtonDown);
    EXPECT_TRUE(events[1].pressed);
    EXPECT_EQ(events[2].type, InputEventType::MouseButtonUp);
    EXPECT_FALSE(events[2].pressed);
    EXPECT_EQ(events[2].x, 12);
}

TEST_F(SDL3InputTest, ClosingTheWindowIsAQuit)
{
    EXPECT_FALSE(input.CheckForQuit());
    SDL_Event e{};
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
    input.Update();
    EXPECT_TRUE(input.CheckForQuit());
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, InputEventType::AppQuit);
}

TEST_F(SDL3InputTest, NothingIsReadWhenNotInitialized)
{
    input.Shutdown();
    Key(SDL_EVENT_KEY_DOWN, SDLK_A);
    input.Update();
    EXPECT_TRUE(events.empty());
    EXPECT_FALSE(input.IsInitialized());
}
