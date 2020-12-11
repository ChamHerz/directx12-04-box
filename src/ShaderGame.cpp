//***************************************************************************************
// directx12-03-contador-frames by ChamHerz (C) 2020
//***************************************************************************************

#include <Windows.h>	//WinMain
#include "Engine.h"

class ShaderGame : public Engine
{
public:
	ShaderGame(HINSTANCE hInstanc);
	~ShaderGame();

	virtual bool initialize() override;

};

int WINAPI WinMain(
	_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ PSTR szCmdLine, _In_ int iCmdShow) {

	// Activa los features para adiministrar la memoria en Debug
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	ShaderGame shaderGame(hInstance);

	// si o si tiene que inicializar, sino algo funciono mal
	if (!shaderGame.initialize())
		return 0;

	return shaderGame.run();
};

ShaderGame::ShaderGame(HINSTANCE hInstance) : Engine(hInstance)
{

}

ShaderGame::~ShaderGame()
{

}

bool ShaderGame::initialize()
{
	if (!Engine::initialize())
		return false;

	return true;
}