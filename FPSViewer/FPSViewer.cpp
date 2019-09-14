//fangzhangmnm 2019/9/13
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#include <vector>
#include <math.h>
#include <ctime>
#include "MQBasePlugin.h"
#include "MQ3DLib.h"
#include "MQWidget.h"
class FPSViewerPlugin;

class FPSViewerWindow : public MQWindow {
public:
	FPSViewerWindow(FPSViewerPlugin *plugin, MQWindowBase& parent);
	BOOL OnTimer(MQWidgetBase *sender, MQDocument doc);
	FPSViewerPlugin *m_pPlugin;
};

class FPSViewerPlugin : public MQCommandPlugin
{
public:
	FPSViewerPlugin();
	virtual void GetPlugInID(DWORD *Product, DWORD *ID);
	virtual const char *GetPlugInName(void);
	virtual const wchar_t *EnumString(void);
	virtual BOOL Initialize();
	virtual void Exit();
	virtual BOOL Activate(MQDocument doc, BOOL flag);
	virtual void OnDraw(MQDocument doc, MQScene scene, int width, int height);
	virtual BOOL OnLeftButtonDown(MQDocument doc, MQScene scene, MOUSE_BUTTON_STATE& state);
	virtual BOOL OnLeftButtonMove(MQDocument doc, MQScene scene, MOUSE_BUTTON_STATE& state);
	virtual BOOL OnLeftButtonUp(MQDocument doc, MQScene scene, MOUSE_BUTTON_STATE& state);
	virtual BOOL OnKeyDown(MQDocument doc, MQScene scene, int key, MOUSE_BUTTON_STATE& state);
	virtual BOOL OnKeyUp(MQDocument doc, MQScene scene, int key, MOUSE_BUTTON_STATE& state);
	virtual BOOL OnMouseMove(MQDocument doc, MQScene scene, MOUSE_BUTTON_STATE& state);
	bool m_bFocusing;
private:
	bool m_wasdeq[6];
	float m_cameraSensitive = 0.05f;
	float m_movementSpeed = 500.0f;
	POINT m_OldCursorPos;
	clock_t m_lastFrameClock;
	FPSViewerWindow* m_pdlgMain;
	MQScene m_activeScene;

};

FPSViewerWindow::FPSViewerWindow(FPSViewerPlugin *plugin, MQWindowBase& parent) : MQWindow(parent)
{
	m_pPlugin = plugin;
	SetTitle(L"FPS Viewer");
	SetOutSpace(0.4);
	SetCloseButton(false);
}

BOOL FPSViewerWindow::OnTimer(MQWidgetBase *sender, MQDocument doc)
{
	return true;//to call redraw
}
FPSViewerPlugin::FPSViewerPlugin()
{
	m_pdlgMain = NULL;
	m_bFocusing = false;
	memset(m_wasdeq, false, sizeof(m_wasdeq));
}

void FPSViewerPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
	*Product = 0x35a7e20c;
	*ID      = 0xc15286c5;
}
const char *FPSViewerPlugin::GetPlugInName(void)
{
	return "FPS Viewer       2019, fangzhangmnm.";
}

const wchar_t *FPSViewerPlugin::EnumString(void)
{
	return L"FPSViewer";
}

BOOL FPSViewerPlugin::Initialize()
{
	if (!m_pdlgMain)
		m_pdlgMain = new FPSViewerWindow(this, MQWindow::GetMainWindow());
	return TRUE;
}

void FPSViewerPlugin::Exit()
{
	if (m_pdlgMain != NULL) {
		delete m_pdlgMain;
		m_pdlgMain = NULL;
	}
}

BOOL FPSViewerPlugin::Activate(MQDocument doc, BOOL flag)
{
	//if(m_pdlgMain)
	//	m_pdlgMain->SetVisible(flag);
	if (!flag){
		memset(m_wasdeq, false, sizeof(m_wasdeq));
		m_bFocusing = false;
	}
	return flag;
}

void FPSViewerPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
	if (m_bFocusing && scene==m_activeScene) {
		clock_t thisFrameClock = clock();
		float dt = ((float)thisFrameClock - m_lastFrameClock) / CLOCKS_PER_SEC;
		m_lastFrameClock = thisFrameClock;
		MQPoint dx(0, 0, 0);
		if (m_wasdeq[0])dx.z -= 1;//w
		if (m_wasdeq[2])dx.z += 1;//d
		if (m_wasdeq[1])dx.x -= 1;//a
		if (m_wasdeq[3])dx.x += 1;//s
		if (m_wasdeq[4])dx.y += 1;//e
		if (m_wasdeq[5])dx.y -= 1;//q
		MQAngle a = scene->GetCameraAngle();
		float deg2rad = 3.14159265 / 180;
		MQPoint forward = MQPoint(sin(a.head*deg2rad)*cos(a.pitch*deg2rad), -sin(a.pitch*deg2rad), -cos(a.head*deg2rad)*cos(a.pitch*deg2rad));
		MQPoint right = MQPoint(cos(a.head*deg2rad), 0, sin(a.head*deg2rad));
		MQPoint up = MQPoint(sin(a.head*deg2rad)*sin(a.pitch*deg2rad), cos(a.pitch*deg2rad), -cos(a.head*deg2rad)*sin(a.pitch*deg2rad));
		dx = -dx.z*forward +dx.x*right + dx.y*up;
		dx = dx*dt*m_movementSpeed;
		scene->SetCameraPosition(scene->GetCameraPosition() + dx);
		scene->SetLookAtPosition(scene->GetCameraPosition() + 200 * forward);
		scene->SetFOV(45.0f);

		//wchar_t buffer[250];
		//swprintf_s(buffer, L"%f %f %f %f %f %f %f %f %f", forward.x, forward.y, forward.z, right.x, right.y, right.z, up.x, up.y, up.z);
		//SetStatusString(buffer);

		MQPoint v[4];
		v[0] = scene->ConvertScreenTo3D(MQPoint(width*.1f,height*.1f, scene->GetFrontZ()));
		v[1] = scene->ConvertScreenTo3D(MQPoint(width*.1f,height*.9f, scene->GetFrontZ()));
		v[2] = scene->ConvertScreenTo3D(MQPoint(width*.9f, height*.9f, scene->GetFrontZ()));
		v[3] = scene->ConvertScreenTo3D(MQPoint(width*.9f, height*.1f, scene->GetFrontZ()));
		int vertex[8];
		MQObject draw = CreateDrawingObject(doc, DRAW_OBJECT_LINE);
		draw->SetColor(MQColor(1, 0, 0));
		draw->SetColorValid(TRUE);
		vertex[7] = vertex[0] = draw->AddVertex(v[0]);
		vertex[1] = vertex[2] = draw->AddVertex(v[1]);
		vertex[3] = vertex[4] = draw->AddVertex(v[2]);
		vertex[5] = vertex[6] = draw->AddVertex(v[3]);
		draw->AddFace(2, vertex + 0);
		draw->AddFace(2, vertex + 2);
		draw->AddFace(2, vertex + 4);
		draw->AddFace(2, vertex + 6);
		if(m_pdlgMain)
			if(m_wasdeq[0] || m_wasdeq[1] || m_wasdeq[2] || m_wasdeq[3] || m_wasdeq[4] || m_wasdeq[5])
				m_pdlgMain->AddTimerEvent(m_pdlgMain, &FPSViewerWindow::OnTimer, 16, true);
	}
}

BOOL FPSViewerPlugin::OnKeyDown(MQDocument doc, MQScene scene, int key, MOUSE_BUTTON_STATE& state) {
	if (!m_bFocusing)return false;
	if (key == 'w' - 'a' + 0x41)
		m_wasdeq[0] = true;
	else if (key == 'a' - 'a' + 0x41) 
		m_wasdeq[1] = true;
	else if (key == 's' - 'a' + 0x41) 
		m_wasdeq[2] = true;
	else if (key == 'd' - 'a' + 0x41) 
		m_wasdeq[3] = true;
	else if (key == 'e' - 'a' + 0x41)
		m_wasdeq[4] = true;
	else if (key == 'q' - 'a' + 0x41)
		m_wasdeq[5] = true;
	else return false;
	if (m_pdlgMain)
		m_pdlgMain->AddTimerEvent(m_pdlgMain, &FPSViewerWindow::OnTimer, 16, true);
	m_lastFrameClock = clock();
	return true;
}
BOOL FPSViewerPlugin::OnKeyUp(MQDocument doc, MQScene scene, int key, MOUSE_BUTTON_STATE& state) {
	if (!m_bFocusing)return false;
	if (key == 'w' - 'a' + 0x41)
		m_wasdeq[0] = false;
	else if (key == 'a' - 'a' + 0x41)
		m_wasdeq[1] = false;
	else if (key == 's' - 'a' + 0x41)
		m_wasdeq[2] = false;
	else if (key == 'd' - 'a' + 0x41)
		m_wasdeq[3] = false;
	else if (key == 'e' - 'a' + 0x41)
		m_wasdeq[4] = false;
	else if (key == 'q' - 'a' + 0x41)
		m_wasdeq[5] = false;
	else return false;
	return true;
}

BOOL FPSViewerPlugin::OnMouseMove(MQDocument doc, MQScene scene, MOUSE_BUTTON_STATE& state) {
	return false;
}
BOOL FPSViewerPlugin::OnLeftButtonDown(MQDocument doc, MQScene scene, MOUSE_BUTTON_STATE& state)
{
	memset(m_wasdeq, false, sizeof(m_wasdeq));
	m_activeScene = scene;
	m_lastFrameClock = clock();
	m_bFocusing = true;
	GetCursorPos(&m_OldCursorPos);
	RedrawScene(scene);
	return true;
}


BOOL FPSViewerPlugin::OnLeftButtonMove(MQDocument doc, MQScene scene, MOUSE_BUTTON_STATE& state)
{
	if (!m_bFocusing)return false;
	POINT CursorPos;
	GetCursorPos(&CursorPos);
	SetCursorPos(m_OldCursorPos.x, m_OldCursorPos.y);
	POINT mousePos = state.MousePos;
	MQAngle cameraAngle = scene->GetCameraAngle();
	cameraAngle.head = fmodf(fmodf( cameraAngle.head+(CursorPos.x - m_OldCursorPos.x)*m_cameraSensitive,360)+360,360);
	cameraAngle.pitch = fmax(fmin(cameraAngle.pitch+(CursorPos.y - m_OldCursorPos.y)*m_cameraSensitive,90),-90);
	scene->SetCameraAngle(cameraAngle);
	RedrawScene(scene);
	return true;
}

BOOL FPSViewerPlugin::OnLeftButtonUp(MQDocument doc, MQScene scene, MOUSE_BUTTON_STATE& state)
{
	//m_bFocusing = false;
	RedrawScene(scene);
	MQAngle a = scene->GetCameraAngle();
	//wchar_t buf[256];
	//swprintf_s(buf, L"%f %f %f", a.head,a.pitch,a.bank);
	//MQDialog::MessageInformationBox(MQWindow::GetMainWindow(), buf, L"FPS Viewer");
	return true;
}
MQBasePlugin *GetPluginClass()
{
	static FPSViewerPlugin plugin;
	return &plugin;
}
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

