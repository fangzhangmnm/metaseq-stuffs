//fangzhangmnm 2019.9

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <vector>
#include "MQBasePlugin.h"
#include "MQ3DLib.h"
#include "MQWidget.h"

#ifdef WIN32
HINSTANCE hInstance;

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	hInstance = (HINSTANCE)hModule;
	return TRUE;
}
#endif


template<class T> class MyBuffer {
public:
	T * m_buffer = NULL; int capacity = 0;
	MyBuffer(int capacity=64) :capacity(capacity), m_buffer(new T[capacity]) {}
	~MyBuffer() { delete[]m_buffer; }
	T& operator[](int i) { return m_buffer[i]; }
	T* createBuffer(int capacity) {
		if (this->capacity < capacity) {
			delete[]m_buffer;
			m_buffer = new T[capacity];
			this->capacity = capacity;
		}
		return m_buffer;
	}
};
class ToonOutlinerPlugin;
class ToonOutlinerWindow : public MQWindow {
public:
	ToonOutlinerWindow(ToonOutlinerPlugin *plugin, MQWindowBase& parent);
	ToonOutlinerPlugin *m_pPlugin;
	MQDoubleSpinBox* edit_width;
	BOOL OnWidthChange(MQWidgetBase*, MQDocument);
	virtual BOOL OnHide(MQWidgetBase * sender, MQDocument doc);
};
class ToonOutlinerPlugin : public MQStationPlugin
{
public:
	ToonOutlinerPlugin();
	~ToonOutlinerPlugin();

#if __APPLE__
	virtual MQBasePlugin *CreateNewPlugin();
#endif
	virtual void GetPlugInID(DWORD *Product, DWORD *ID);
	virtual const char *GetPlugInName(void);
	virtual const wchar_t *EnumString(void);
	virtual BOOL Initialize();
	virtual void Exit();
	virtual BOOL Activate(MQDocument doc, BOOL flag);
	virtual BOOL IsActivated(MQDocument doc);
	virtual void OnDraw(MQDocument doc, MQScene scene, int width, int height);
	virtual void OnUpdateObjectList(MQDocument doc);
	virtual void OnObjectModified(MQDocument doc);
	virtual void OnNewDocument(MQDocument doc);
	virtual void OnEndDocument(MQDocument doc);
	void OnWidthChange(MQDocument doc);

	bool m_bActivate;
protected:
	virtual bool ExecuteCallback(MQDocument doc, void *option);

private:
	void extrude(MQObject obj, float distance);
	void setMaterial(MQObject obj, int matId);
	void invertFace(MQObject obj);
	void createDrawingObjects(MQDocument doc, int instance);
	std::vector<MQObject> drawingObjects;
	std::vector<MQMaterial> drawingMaterials;
	MQDocument drawingDocument = NULL;
	void clearDrawingObjects(MQDocument doc);
	ToonOutlinerWindow* m_pdlgMain;
};


MQBasePlugin *GetPluginClass()
{
	static ToonOutlinerPlugin plugin;
	return &plugin;
}

ToonOutlinerPlugin::ToonOutlinerPlugin()
{
	m_pdlgMain = NULL;
	m_bActivate = false;
}

ToonOutlinerPlugin::~ToonOutlinerPlugin()
{
	if (drawingDocument)
		clearDrawingObjects(drawingDocument);
	if (m_pdlgMain != NULL) {
		delete m_pdlgMain;
		m_pdlgMain = NULL;
	}
}

#if __APPLE__
MQBasePlugin *IndexerPlugin::CreateNewPlugin()
{
	return new IndexerPlugin();
}
#endif

void ToonOutlinerPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
	*Product = 0x35a7e20c;
	*ID = 0x2b46cf22;
}

const char *ToonOutlinerPlugin::GetPlugInName(void)
{
	return "Toon Outliner       fangzhangmnm 2019";
}

const wchar_t *ToonOutlinerPlugin::EnumString(void)
{
	return L"Toon Outliner";
}

BOOL ToonOutlinerPlugin::Initialize()
{
	if (!m_pdlgMain)
		m_pdlgMain = new ToonOutlinerWindow(this, MQWindow::GetMainWindow());
	return TRUE;
}

void ToonOutlinerPlugin::Exit()
{
	if (m_pdlgMain != NULL) {
		delete m_pdlgMain;
		m_pdlgMain = NULL;
	}
}
BOOL ToonOutlinerPlugin::Activate(MQDocument doc, BOOL flag)
{
	if (m_pdlgMain)
		m_pdlgMain->SetVisible(flag);
	m_bActivate = flag;
	if (drawingDocument != NULL)clearDrawingObjects(doc);
	if(flag) createDrawingObjects(doc,0);
	MQ_RefreshView(NULL);
	return flag;
}

BOOL ToonOutlinerPlugin::IsActivated(MQDocument doc)
{
	return m_bActivate;
}



void ToonOutlinerPlugin::OnDraw(MQDocument doc, MQScene scene, int width, int height)
{
	if (!m_bActivate)return;
}
void ToonOutlinerPlugin::createDrawingObjects(MQDocument doc,int instance=1) {
	assert(drawingDocument == NULL);
	drawingDocument = doc;
	int matId;
	MQMaterial mat = CreateDrawingMaterial(doc, matId, instance);
	mat->SetShader(MQMATERIAL_SHADER_CONSTANT);
	mat->SetColor(MQColor(0, 0, 0));
	if(instance==0)drawingMaterials.push_back(mat);
	for (int iterObj = 0; iterObj < doc->GetObjectCount(); ++iterObj) {
		MQObject obj = doc->GetObject(iterObj);
		if (obj->GetVisible()) {
			MQObject obj2 = CreateDrawingObjectByClone(doc, obj, DRAW_OBJECT_FACE, instance);
			if (instance == 0)drawingObjects.push_back(obj2);
			setMaterial(obj2, matId);
			extrude(obj2, m_pdlgMain->edit_width->GetPosition());
			invertFace(obj2);
		}
	}
}
void ToonOutlinerPlugin::clearDrawingObjects(MQDocument doc)
{
	assert(doc == drawingDocument);
	for (MQObject o : drawingObjects)
		DeleteDrawingObject(doc, o);
	drawingObjects.clear();
	for (MQMaterial m : drawingMaterials)
		DeleteDrawingMaterial(doc, m);
	drawingMaterials.clear();
	drawingDocument = NULL;
}
void ToonOutlinerPlugin::setMaterial(MQObject obj, int matId) {
	for (int i = 0; i < obj->GetFaceCount(); ++i)obj->SetFaceMaterial(i, matId);
}
void ToonOutlinerPlugin::invertFace(MQObject obj) {
	for (int i = 0; i < obj->GetFaceCount(); ++i)obj->InvertFace(i);
}
void ToonOutlinerPlugin::extrude(MQObject obj, float distance) {
	MyBuffer<int> fpa;
	int n = obj->GetVertexCount();
	MQPoint* normals = new MQPoint[n];
	for (int i = 0; i < n; ++i)normals[i] = MQPoint(0, 0, 0);

	int nf = obj->GetFaceCount();
	for (int i = 0; i < nf; ++i) {
		int np = obj->GetFacePointCount(i);
		obj->GetFacePointArray(i, fpa.createBuffer(np));
		for (int j = 0; j < np; ++j) {
			int k = fpa[j];
			MQPoint normal;BYTE flag;
			obj->GetFaceVertexNormal(i, j, flag, normal);
			normals[k] += normal;
		}
	}
	for (int i = 0; i < n; ++i) {
		normals[i].normalize();
		obj->SetVertex(i,obj->GetVertex(i)+ normals[i] * distance);
	}
	delete[]normals;
}

void ToonOutlinerPlugin::OnUpdateObjectList(MQDocument doc)
{
	if (drawingDocument != NULL)clearDrawingObjects(doc);
	if (m_bActivate)createDrawingObjects(doc, 0);
	MQ_RefreshView(NULL);
}
void ToonOutlinerPlugin::OnObjectModified(MQDocument doc)
{
	if (drawingDocument != NULL)clearDrawingObjects(doc);
	if (m_bActivate)createDrawingObjects(doc,0);
	MQ_RefreshView(NULL);
}

void ToonOutlinerPlugin::OnNewDocument(MQDocument doc)
{
	if (drawingDocument != NULL)clearDrawingObjects(doc);
	if (m_bActivate)createDrawingObjects(doc,0);
	MQ_RefreshView(NULL);
}
void ToonOutlinerPlugin::OnEndDocument(MQDocument doc)
{
	if (drawingDocument != NULL)clearDrawingObjects(doc);
	MQ_RefreshView(NULL);
}

void ToonOutlinerPlugin::OnWidthChange(MQDocument doc)
{
	if (drawingDocument != NULL)clearDrawingObjects(doc);
	if (m_bActivate)createDrawingObjects(doc, 0);
	MQ_RefreshView(NULL);

}

bool ToonOutlinerPlugin::ExecuteCallback(MQDocument doc, void *option)
{
	return false;
}

ToonOutlinerWindow::ToonOutlinerWindow(ToonOutlinerPlugin * plugin, MQWindowBase & parent)
	:MQWindow(parent),m_pPlugin(plugin)
{
	SetTitle(L"Toon Outliner");
	SetOutSpace(0.4);
	SetCloseButton(true);
	SetCanResize(false);
	MQFrame *mainFrame = CreateHorizontalFrame(this);
	MQFrame *paramFrame = CreateHorizontalFrame(mainFrame);
	paramFrame->SetMatrixColumn(2);
	CreateLabel(paramFrame, L"Width");
	edit_width=CreateDoubleSpinBox(paramFrame);
	edit_width->SetPosition(0.5f);
	edit_width->SetIncrement(0.1f);
	edit_width->AddChangedEvent(this, &ToonOutlinerWindow::OnWidthChange);
}

BOOL ToonOutlinerWindow::OnWidthChange(MQWidgetBase *, MQDocument doc)
{
	m_pPlugin->OnWidthChange(doc);
	return false;
}

BOOL ToonOutlinerWindow::OnHide(MQWidgetBase *sender, MQDocument doc)
{
	if (m_pPlugin->m_bActivate) {
		m_pPlugin->m_bActivate = false;
		m_pPlugin->WindowClose();
	}
	return FALSE;
}
