#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/Sound.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/DX11Renderer.hpp"
#include "Engine/Window/Window.hpp"


//-----------------------------------------------------------------------------------------------
App*			g_theApp		= nullptr;		// Created and owned by Main_Windows.cpp
Window*			g_theWindow		= nullptr;		// Created and owned by the App
Renderer*		g_theRenderer	= nullptr;		// Created and owned by the App
AudioSystem*    g_theAudio		= nullptr;		// Created and owned by the App
Game*			g_theGame		= nullptr;
bool			g_isDebugDraw	= false;
RandomNumberGenerator g_rng;

//-----------------------------------------------------------------------------------------------
App::App()
{
}

App::~App()
{
}

void App::Startup()
{
	// Parse Data/GameConfig.xml
	LoadGameConfig("Data/GameConfig.xml");
	// Create all Engine subsystems
	EventSystemConfig eventSystemConfig;
	g_theEventSystem = new EventSystem(eventSystemConfig);

	InputConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	//windowConfig.m_aspectRatio = 2.f;
	windowConfig.m_aspectRatio = g_gameConfigBlackboard.GetValue("windowAspect", 2.f);
	windowConfig.m_windowTitle = g_gameConfigBlackboard.GetValue("windowTitle", "Protogame2D");
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;
	g_theRenderer = new DX11Renderer(rendererConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_defaultRenderer = g_theRenderer;
	devConsoleConfig.m_fontName = g_gameConfigBlackboard.GetValue("fontName", devConsoleConfig.m_fontName);
	devConsoleConfig.m_fontAspectScale = 0.8f;
	g_theDevConsole = new DevConsole(devConsoleConfig);

	AudioConfig audioConfig;
	g_theAudio = new AudioSystem(audioConfig);

	// Start up all Engine subsystems
	g_theEventSystem->Startup();
	g_theDevConsole->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	g_theAudio->Startup();

	// Initialize game-related stuff: create and start the game
	Sound::Init();
	g_theGame = new Game();
	g_theEventSystem->SubscribeEventCallbackFunction("Quit", OnQuitEvent);
}

void App::Shutdown()
{
	// Destroy game-related stuff
	delete g_theGame;
	g_theGame = nullptr;

	// Shut down all Engine subsystems
	g_theAudio->Shutdown();
	g_theDevConsole->Shutdown();
	g_theRenderer->Shutdown();
	g_theWindow->Shutdown();
	g_theInput->Shutdown();
	g_theEventSystem->Shutdown();

	// Destroy all engine subsystems
	delete g_theAudio;
	g_theAudio = nullptr;
	delete g_theRenderer;
	g_theRenderer = nullptr;
	delete g_theWindow;
	g_theWindow = nullptr;
	delete g_theInput;
	g_theInput = nullptr;
	delete g_theEventSystem;
	g_theEventSystem = nullptr;
}

void App::RunMainLoop()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

void App::RunFrame()
{
	double currentTime = GetCurrentTimeSeconds();
	float deltaSeconds = static_cast<float>(currentTime - m_timeLastFrameStart);
	deltaSeconds = (deltaSeconds > 0.1f) ? 0.1f : deltaSeconds;
	m_timeLastFrameStart = currentTime;

	BeginFrame();			// Engine pre-frame stuff
	Update(deltaSeconds);	// Game updates / moves/ spawns / hurts/ kills stuffs
	Render();				// Game draws current state of things
	EndFrame();				// Engine post-frame stuff
}

void App::HandleQuitRequested()
{
	m_isQuitting = true;
}

void App::BeginFrame()
{
	g_theEventSystem->BeginFrame();
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theDevConsole->BeginFrame();
	g_theAudio->BeginFrame();
}

void App::Update(float deltaSeconds)
{
	// the Game can only be hard reset by F8, do not delete and create Game except this
	// Because the Game will reload the resource
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		delete g_theGame;
		g_theGame = new Game();
	}
	
	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE))
	{
		g_theDevConsole->ToggleMode(DevConsoleMode::OPEN_FULL);
	}

	g_theGame->Update(deltaSeconds);
}

void App::Render() const
{
	g_theRenderer->ClearScreen(Rgba8(180, 180, 80));
	g_theGame->Render();
	g_theDevConsole->Render(AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void App::EndFrame()
{
	g_theAudio->EndFrame();
	g_theDevConsole->EndFrame();
	g_theRenderer->EndFrame();
	g_theWindow->EndFrame();
    g_theInput->EndFrame();
	g_theEventSystem->EndFrame();
}

void App::LoadGameConfig(char const* gameConfigXmlFilePath)
{
	XmlDocument gameConfigXml;
	XmlResult result = gameConfigXml.LoadFile(gameConfigXmlFilePath);
	if (result != tinyxml2::XML_SUCCESS)
	{
		// TODO Debugger
		DebuggerPrintf("WARNING: failed to load game config from file \"%s\"\n", gameConfigXmlFilePath);
		return;
	}
	XmlElement* rootElement = gameConfigXml.RootElement();
	if (rootElement == nullptr)
	{
		DebuggerPrintf("WARNING: game config from file \"%s\" was invalid (missing root element)\n", gameConfigXmlFilePath);
		return;
	}
	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);

}

bool OnQuitEvent(EventArgs& args)
{
	UNUSED(args);
	g_theApp->HandleQuitRequested();
	return false;
}
